// ============================================================================
// FILTRO DE KALMAN ESCALAR DE 2 ESTADOS - MPU6050 + ESP32
// Implementacao rigorosa conforme modelo teorico
//
// Vetor de estado:  x = [theta, bias]^T
// F = [[1, -dt], [0, 1]]   H = [1, 0]
// Q = diag(Q_theta*dt, Q_b*dt)   R escalar
//
// Serial Plotter: 2 curvas (bruto vs filtrado) por eixo
// Alterar MODO_PLOT para escolher o eixo ou mostrar ambos
// ============================================================================

#include <Wire.h>
#include <MPU6050_light.h>
#include "esp_timer.h"

// ============================================================
// CONFIGURACAO DE HARDWARE
// ============================================================
#define PINO_SDA     22
#define PINO_SCL     21
#define MPU_ADDR     0x68
#define INTERVALO_US 5000         // 200 Hz
#define BAUDRATE     115200

// ============================================================
// MODO DE PLOT
// 0 = 2 curvas de Roll   (Roll_Bruto,   Roll_Kalman)
// 1 = 2 curvas de Pitch  (Pitch_Bruto,  Pitch_Kalman)
// 2 = 4 curvas (Roll + Pitch)
// ============================================================
#define MODO_PLOT 0

// ============================================================
// PARAMETROS EXPERIMENTAIS CALIBRADOS
// (mediana das 5 afericoes validas - ver relatorio)
// ============================================================
const float R_ROLL     = 0.0125f;      // (graus)^2
const float R_PITCH    = 0.01585f;     // (graus)^2
const float Q_THETA_X  = 0.00001337f;  // (graus)^2 por passo
const float Q_THETA_Y  = 0.00001566f;  // (graus)^2 por passo
const float Q_B_X      = 0.000010f;    // (graus/s)^2 por segundo
const float Q_B_Y      = 0.000048f;    // (graus/s)^2 por segundo
const float B0_X       = 0.0061f;      // graus/s (bias inicial)
const float B0_Y       = 0.0032f;      // graus/s
const float P_11_0_X   = 0.000050f;    // (graus/s)^2 (incerteza inicial do bias)
const float P_11_0_Y   = 0.000050f;

// Fator de R em movimento (compensa aceleracao linear)
const float FATOR_R_MOVIMENTO = 3.0f;

// ============================================================
// ESTRUTURA DO FILTRO DE KALMAN (2 ESTADOS)
// ============================================================
struct Kalman2Estados {
    // Parametros
    float Q_theta;   // variancia do ruido do processo no angulo
    float Q_b;       // variancia do ruido do processo no bias
    float R;         // variancia do ruido da medicao

    // Estado
    float theta;     // angulo estimado (graus)
    float bias;      // bias estimado (graus/s)

    // Covariancia (matriz 2x2 simetrica)
    float P00, P01, P10, P11;

    // --------------------------------------------------------
    // Inicializacao: x0 e P0
    // --------------------------------------------------------
    void init(float q_theta, float q_b, float r,
              float theta0, float bias0,
              float p00_0, float p11_0) {
        Q_theta = q_theta;
        Q_b     = q_b;
        R       = r;

        theta = theta0;
        bias  = bias0;

        P00 = p00_0;
        P01 = 0.0f;
        P10 = 0.0f;
        P11 = p11_0;
    }

    // --------------------------------------------------------
    // PREDICAO (a priori)
    //   theta_pred = theta + (omega - bias) * dt
    //   bias_pred  = bias
    //   P_pred     = F * P * F^T + Q
    // --------------------------------------------------------
    void predict(float omega, float dt) {
        // --- Estado ---
        theta = theta + (omega - bias) * dt;
        // bias permanece inalterado

        // --- Covariancia: F*P*F^T + Q ---
        float P00_new = P00 + dt * (dt * P11 - P01 - P10 + Q_theta);
        float P01_new = P01 - dt * P11;
        float P10_new = P10 - dt * P11;
        float P11_new = P11 + Q_b;

        P00 = P00_new;
        P01 = P01_new;
        P10 = P10_new;
        P11 = P11_new;
    }

    // --------------------------------------------------------
    // CORRECAO (a posteriori)
    //   S = H*P*H^T + R         (escalar)
    //   K = P*H^T / S           (vetor 2x1)
    //   x = x_pred + K*y        com y = z - H*x_pred
    //   P = (I - K*H)*P_pred
    // --------------------------------------------------------
    void update(float z, float R_efetivo) {
        // Ganho de Kalman
        float S  = P00 + R_efetivo;
        float K0 = P00 / S;
        float K1 = P10 / S;

        // Inovacao
        float y = z - theta;

        // Correcao do estado
        theta = theta + K0 * y;
        bias  = bias  + K1 * y;

        // Correcao da covariancia (usando valores PRE-correcao)
        float P00_pred = P00;
        float P01_pred = P01;

        P00 = P00_pred - K0 * P00_pred;
        P01 = P01_pred - K0 * P01_pred;
        P10 = P10      - K1 * P00_pred;
        P11 = P11      - K1 * P01_pred;
    }
};

// ============================================================
// OBJETOS GLOBAIS
// ============================================================
Kalman2Estados kf_roll;
Kalman2Estados kf_pitch;
MPU6050 mpu(Wire);

volatile bool flag_nova_amostra = false;
int64_t ultimo_tempo_us = 0;
esp_timer_handle_t timer_handle;
bool inicializado = false;

// ============================================================
// CALLBACK DO TIMER
// ============================================================
void IRAM_ATTR timer_callback(void *arg) {
    flag_nova_amostra = true;
}

// ============================================================
// DETECCAO DE MOVIMENTO (para ajuste dinamico de R)
// ============================================================
bool detectar_movimento(float ax, float ay, float az,
                        float gx, float gy, float gz) {
    float acc_mag  = sqrtf(ax*ax + ay*ay + az*az);
    float giro_mag = sqrtf(gx*gx + gy*gy + gz*gz);
    return (fabsf(acc_mag - 1.0f) > 0.15f) || (giro_mag > 10.0f);
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(BAUDRATE);
    delay(2000);

    // Cabecalho para o Serial Plotter (define os labels das curvas)
    #if   MODO_PLOT == 0
        Serial.println("Roll_Bruto,Roll_Kalman");
    #elif MODO_PLOT == 1
        Serial.println("Pitch_Bruto,Pitch_Kalman");
    #else
        Serial.println("Roll_Bruto,Roll_Kalman,Pitch_Bruto,Pitch_Kalman");
    #endif

    // I2C
    Wire.begin(PINO_SDA, PINO_SCL);
    Wire.setClock(400000);

    Wire.beginTransmission(MPU_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("ERRO I2C");
        while (1) delay(1000);
    }

    if (mpu.begin() != 0) {
        Serial.println("ERRO MPU6050");
        while (1) delay(1000);
    }

    // DLPF = 2 (94 Hz)
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x1A);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 1);
    uint8_t cfg = Wire.read();
    cfg = (cfg & 0b11111000) | 0b00000010;
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x1A);
    Wire.write(cfg);
    Wire.endTransmission(true);

    // Calibracao de offsets (sensor DEVE estar parado)
    delay(3000);
    mpu.calcOffsets();

    // --- Inicializacao rigorosa dos filtros ---
    // theta0 sera preenchido com a primeira leitura do acelerometro
    // P00_0 = R (confia na leitura inicial)
    // P11_0 = P_11,0 medido experimentalmente
    kf_roll.init( Q_THETA_X, Q_B_X, R_ROLL,
                  0.0f, B0_X, R_ROLL, P_11_0_X );
    kf_pitch.init(Q_THETA_Y, Q_B_Y, R_PITCH,
                  0.0f, B0_Y, R_PITCH, P_11_0_Y );

    // Timer 200 Hz
    const esp_timer_create_args_t args = {
        .callback = &timer_callback,
        .arg      = NULL,
        .name     = "timer_200hz"
    };
    esp_timer_create(&args, &timer_handle);
    esp_timer_start_periodic(timer_handle, INTERVALO_US);

    ultimo_tempo_us = esp_timer_get_time();
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================
void loop() {
    if (!flag_nova_amostra) return;
    flag_nova_amostra = false;

    // --- dt real medido com esp_timer ---
    int64_t agora = esp_timer_get_time();
    float dt = (agora - ultimo_tempo_us) / 1e6f;
    ultimo_tempo_us = agora;
    if (dt <= 0.0f || dt > 0.1f) return;

    // --- Leitura do MPU ---
    mpu.update();
    float ax = mpu.getAccX();
    float ay = mpu.getAccY();
    float az = mpu.getAccZ();
    float gx = mpu.getGyroX();
    float gy = mpu.getGyroY();
    float gz = mpu.getGyroZ();

    // --- Angulos medidos pelo acelerometro ---
    float roll_acc  = atan2f(ay, az)                    * 180.0f / PI;
    float pitch_acc = atan2f(-ax, sqrtf(ay*ay + az*az)) * 180.0f / PI;

    // --- Inicializacao: theta0 = primeira leitura ---
    if (!inicializado) {
        kf_roll.theta  = roll_acc;
        kf_pitch.theta = pitch_acc;
        inicializado   = true;
        return;
    }

    // --- Ajuste dinamico de R (compensa aceleracao linear) ---
    bool mov = detectar_movimento(ax, ay, az, gx, gy, gz);
    float R_roll_ef  = mov ? R_ROLL  * FATOR_R_MOVIMENTO : R_ROLL;
    float R_pitch_ef = mov ? R_PITCH * FATOR_R_MOVIMENTO : R_PITCH;

    // --- Roll ---
    #if MODO_PLOT == 0 || MODO_PLOT == 2
        kf_roll.predict(gx, dt);
        kf_roll.update(roll_acc, R_roll_ef);
    #endif

    // --- Pitch ---
    #if MODO_PLOT == 1 || MODO_PLOT == 2
        kf_pitch.predict(gy, dt);
        kf_pitch.update(pitch_acc, R_pitch_ef);
    #endif

    // --- Saida CSV para Serial Plotter ---
    #if   MODO_PLOT == 0
        Serial.printf("%.3f,%.3f\n",
                      roll_acc, kf_roll.theta);
    #elif MODO_PLOT == 1
        Serial.printf("%.3f,%.3f\n",
                      pitch_acc, kf_pitch.theta);
    #else
        Serial.printf("%.3f,%.3f,%.3f,%.3f\n",
                      roll_acc,   kf_roll.theta,
                      pitch_acc,  kf_pitch.theta);
    #endif
}
