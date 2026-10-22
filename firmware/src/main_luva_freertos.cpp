#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define NUM_DEDOS      5
#define NUM_ESTADOS   20

//-----------------Parâmetros do filtro---------------------

const float FS = 50.0f;                             // taxa de amostragem (Hz)
const float FC = 5.0f;                              // frequência de corte (Hz)
const uint32_t TS_MS = (uint32_t)(1000.0f / FS);   // período em ms (20 ms)

const float HIST = 0.2f;                            // histerese entre estados

// Coeficientes de H(z)
float b0, b1, b2, a1, a2;

//--------------------Pinos-------------------------

const int pinosFlex[NUM_DEDOS] =
{
    34, // dedao    (ADC1)
    35, // indicador(ADC1)
    32, // meio     (ADC1)
    33, // anelar   (ADC1)
    36  // mindinho (ADC1 / VP)
};

//----------------Estruturas-------------------------

typedef struct
{
    float w1, w2;
    bool  iniciado;
} Filtro;

typedef struct
{
    int   adc;        // leitura bruta
    float filtrado;   // saída do filtro
    int minimo;
    int maximo;
    int estado;
    Filtro f;         // memória própria deste dedo
} Flex;

typedef struct
{
    Flex dedo0; // dedão
    Flex dedo1; // indicador
    Flex dedo2; // do meio
    Flex dedo3; // anelar
    Flex dedo4; // mindinho
} Mao;

Mao mao;

typedef struct
{
    float accelX, accelY, accelZ;
    float gyroX, gyroY, gyroZ;
} DadosMPU;

DadosMPU mpuDados;
Adafruit_MPU6050 mpu;

//-------------------------------------------------

TaskHandle_t TaskFlex;
TaskHandle_t TaskMPU;

SemaphoreHandle_t xMaoMutex;
SemaphoreHandle_t xMpuMutex; // Mutex para proteger leituras/escritas do MPU

//------------------Filtro-------------------------

void calculaCoeficientes()
{
    float K  = tanf(PI * FC / FS);
    float K2 = K * K;
    float r2 = sqrtf(2.0f) * K;
    float D  = 1.0f + r2 + K2;

    b0 = K2 / D;
    b1 = 2.0f * K2 / D;
    b2 = K2 / D;

    a1 = 2.0f * (K2 - 1.0f) / D;
    a2 = (1.0f - r2 + K2) / D;
}

float filtro(Filtro *f, float u)
{
    if(!f->iniciado)
    {
        f->w1 = f->w2 = u / (1.0f + a1 + a2);
        f->iniciado = true;
    }

    float w = u - a1 * f->w1 - a2 * f->w2;
    float y = b0 * w + b1 * f->w1 + b2 * f->w2;

    f->w2 = f->w1;
    f->w1 = w;

    return y;
}

//-------------------------------------------------

int adcParaEstado(Flex *f)
{
    float faixa = (float)(f->maximo - f->minimo);
    if(faixa <= 0.0f)
        return f->estado;

    float p = (f->filtrado - f->minimo) / faixa;

    if(p < 0.0f) p = 0.0f;
    if(p > 1.0f) p = 1.0f;

    float pos = p * (NUM_ESTADOS - 1);

    if(pos > f->estado + 0.5f + HIST || pos < f->estado - 0.5f - HIST)
        return (int)(pos + 0.5f);

    return f->estado;
}

void processaDedo(Flex *f, int pino)
{
    f->adc      = analogRead(pino);
    f->filtrado = filtro(&f->f, (float)f->adc);
    f->estado   = adcParaEstado(f);
}

//-------------------------------------------------

void atualizarSensores()
{
    Flex novo0, novo1, novo2, novo3, novo4;

    novo0 = mao.dedo0;  processaDedo(&novo0, pinosFlex[0]);
    novo1 = mao.dedo1;  processaDedo(&novo1, pinosFlex[1]);
    novo2 = mao.dedo2;  processaDedo(&novo2, pinosFlex[2]);
    novo3 = mao.dedo3;  processaDedo(&novo3, pinosFlex[3]);
    novo4 = mao.dedo4;  processaDedo(&novo4, pinosFlex[4]);

    if(xSemaphoreTake(xMaoMutex, pdMS_TO_TICKS(10)))
    {
        mao.dedo0 = novo0;
        mao.dedo1 = novo1;
        mao.dedo2 = novo2;
        mao.dedo3 = novo3;
        mao.dedo4 = novo4;

        xSemaphoreGive(xMaoMutex);
    }
}

//-------------------------------------------------

void imprimirEstados()
{
    Mao maoLocal;
    DadosMPU mpuLocal;

    // Cópia segura da Luva
    if(xSemaphoreTake(xMaoMutex, pdMS_TO_TICKS(10)))
    {
        maoLocal = mao;
        xSemaphoreGive(xMaoMutex);
    }

    // Cópia segura do MPU
    if(xSemaphoreTake(xMpuMutex, pdMS_TO_TICKS(10)))
    {
        mpuLocal = mpuDados;
        xSemaphoreGive(xMpuMutex);
    }

    Serial.print("Flex -> ");
    Serial.print(maoLocal.dedo0.estado); Serial.print(" ");
    Serial.print(maoLocal.dedo1.estado); Serial.print(" ");
    Serial.print(maoLocal.dedo2.estado); Serial.print(" ");
    Serial.print(maoLocal.dedo3.estado); Serial.print(" ");
    Serial.print(maoLocal.dedo4.estado);

    Serial.print(" | Accel -> ");
    Serial.print(mpuLocal.accelX, 2); Serial.print(" ");
    Serial.print(mpuLocal.accelY, 2); Serial.print(" ");
    Serial.print(mpuLocal.accelZ, 2);

    Serial.print(" | Gyro -> ");
    Serial.print(mpuLocal.gyroX, 2); Serial.print(" ");
    Serial.print(mpuLocal.gyroY, 2); Serial.print(" ");
    Serial.print(mpuLocal.gyroZ, 2);

    Serial.println();
}

//-------------------------------------------------

void inicializaDedo(Flex *f)
{
    f->minimo = 800;
    f->maximo = 3300;
    f->estado = 0;
    f->f.iniciado = false;
}

void inicializarMao()
{
    inicializaDedo(&mao.dedo0);
    inicializaDedo(&mao.dedo1);
    inicializaDedo(&mao.dedo2);
    inicializaDedo(&mao.dedo3);
    inicializaDedo(&mao.dedo4);
}

//--------------------Tasks-------------------------

void TaskLeituraFlex(void *pv)
{
    TickType_t ultimo = xTaskGetTickCount();

    while(true)
    {
        atualizarSensores();
        vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(TS_MS));
    }
}

void TaskLeituraMPU(void *pv)
{
    Wire.begin(21, 22);

    if (!mpu.begin()) {
        Serial.println("Falha ao encontrar MPU6050!");
        while (1) { vTaskDelay(pdMS_TO_TICKS(100)); }
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    sensors_event_t a, g, temp;

    while(true)
    {
        mpu.getEvent(&a, &g, &temp);

        if(xSemaphoreTake(xMpuMutex, pdMS_TO_TICKS(10)))
        {
            mpuDados.accelX = a.acceleration.x;
            mpuDados.accelY = a.acceleration.y;
            mpuDados.accelZ = a.acceleration.z;

            mpuDados.gyroX  = g.gyro.x;
            mpuDados.gyroY  = g.gyro.y;
            mpuDados.gyroZ  = g.gyro.z;

            xSemaphoreGive(xMpuMutex);
        }

        // Imprime os dados consolidados
        imprimirEstados();

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

//-------------------------------------------------

void setup()
{
    Serial.begin(115200);

    analogReadResolution(12);

    calculaCoeficientes();

    xMaoMutex = xSemaphoreCreateMutex();
    xMpuMutex = xSemaphoreCreateMutex();

    inicializarMao();

    // Core 0: Leitura e filtragem dos sensores Flex
    xTaskCreatePinnedToCore(
        TaskLeituraFlex,
        "Flex",
        4096,
        NULL,
        1,
        &TaskFlex,
        0
    );

    // Core 1: Leitura da MPU6050 e Impressão
    xTaskCreatePinnedToCore(
        TaskLeituraMPU,
        "MPU",
        4096,
        NULL,
        1,
        &TaskMPU,
        1
    );
}

void loop()
{
    vTaskDelete(NULL);
}
