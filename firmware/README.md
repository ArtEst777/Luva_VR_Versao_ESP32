# Firmware Embarcado — L-UVA (ESP32 + MPU6050 + Sensores Flex)

Este diretório contém a pilha de software embarcado desenvolvida para a **L-UVA**, responsável pela aquisição determinística em tempo real, filtragem digital de sinais analógicos, fusão sensorial inercial e transmissão serial para o motor de simulação 3D (Unity).

---

## Estrutura do Diretório

```text
firmware/
├── src/
│   └── main_luva_freertos.cpp        # Firmware principal multitarefa (Dual-Core FreeRTOS + Filtro Butterworth + Histerese)
├── kalman_imu/
│   └── kalman_2_estados_mpu6050.cpp  # Fusão sensorial de alta frequência (200 Hz) via Filtro de Kalman de 2 Estados
└── test_bench/
    └── validacao_adc_sensores.ino    # Firmware de inspeção elétrica e validação individual dos canais analógicos
```

---

## 1. Arquitetura Multitarefa em Tempo Real (`src/main_luva_freertos.cpp`)

Para garantir baixa latência e determinismo temporal sem bloqueios de I/O entre a leitura analógica dos dedos e a comunicação $\text{I}^2\text{C}$ com a IMU, o firmware explora os dois núcleos físicos da **ESP32 (Xtensa Dual-Core LX6)** por meio do sistema operacional de tempo real **FreeRTOS**:

- **Núcleo 0 (`TaskLeituraFlex`)**: Dedicado exclusivamente à amostragem periódica dos sensores de flexão a exatos **$50\text{ Hz}$** ($T_s = 20\text{ ms}$) utilizando temporização determinística via `vTaskDelayUntil`.
- **Núcleo 1 (`TaskLeituraMPU`)**: Dedicado à comunicação $\text{I}^2\text{C}$ com o módulo inercial **MPU6050** ($\pm 8g$ para acelerômetro, $\pm 500^\circ/\text{s}$ para giroscópio) e consolidação do frame de telemetria serial a $115200\text{ bps}$.
- **Sincronização Thread-Safe**: O acesso às estruturas globais da mão (`xMaoMutex`) e da IMU (`xMpuMutex`) é protegido por **Mutexes** (`SemaphoreHandle_t`), eliminando condições de corrida (*race conditions*) durante a serialização dos estados.

### 1.1. Filtro Digital Butterworth de 2ª Ordem
Os sinais brutos provenientes do ADC de 12 bits ($0$ a $4095$) passam por um filtro passa-baixas digital **Butterworth de 2ª ordem** com frequência de corte $f_c = 5\text{ Hz}$ e taxa de amostragem $f_s = 50\text{ Hz}$, projetado via transformação bilinear:

$$H(z) = \frac{b_0 + b_1 z^{-1} + b_2 z^{-2}}{1 + a_1 z^{-1} + a_2 z^{-2}}$$

O filtro conta com inicialização automática de estado estacionário ($w_1 = w_2 = u / (1 + a_1 + a_2)$) na primeira amostra para evitar transitórios de inicialização.

### 1.2. Discretização em Estados com Histerese
Após a filtragem, a leitura de cada dedo é normalizada dentro dos limites calibrados (`minimo` e `maximo`) e quantizada em **$20$ estados discretos** ($0$ a $19$). Uma banda de **histerese ($\pm 0,2$)** impede oscilações espúrias (*chattering*) quando o dedo permanece estacionário na fronteira entre dois estados adjacentes.

---

## 2. Fusão Sensorial via Filtro de Kalman (`kalman_imu/kalman_2_estados_mpu6050.cpp`)

Para corrigir a deriva (*drift*) de integração do giroscópio e o ruído de alta frequência do acelerômetro, o módulo de fusão sensorial implementa um **Filtro de Kalman Escalar de 2 Estados** operando a **$200\text{ Hz}$** ($\Delta t = 5000\ \mu\text{s}$ gerenciado por interrupção de hardware via `esp_timer`):

- **Vetor de Estado**: $\mathbf{x}_k = [\theta_k, \ b_k]^T$, estimando simultaneamente o ângulo de inclinação ($\text{Roll}$ / $\text{Pitch}$) e o *bias* instantâneo do giroscópio.
- **Matrizes do Modelo**:
  $$\mathbf{F} = \begin{bmatrix} 1 & -\Delta t \\ 0 & 1 \end{bmatrix}, \quad \mathbf{H} = \begin{bmatrix} 1 & 0 \end{bmatrix}$$
- **Covariância de Medição Adaptativa ($R$ Dinâmico)**: O algoritmo monitora a norma do vetor de aceleração e velocidade angular (`detectar_movimento`). Sob acelerações lineares bruscas, a variância $R$ é escalonada pelo fator `FATOR_R_MOVIMENTO = 3.0`, reduzindo temporariamente o ganho sobre o acelerômetro e preservando a estabilidade da orientação.

---

## 3. Dependências e Compilação

### Bibliotecas Necessárias (Arduino IDE / PlatformIO)
- `Adafruit MPU6050` e `Adafruit Unified Sensor` (para `main_luva_freertos.cpp`)
- `MPU6050_light` (para `kalman_2_estados_mpu6050.cpp`)
- `Wire` (nativa do core ESP32)

### Parâmetros de Gravação
- **Placa**: `DOIT ESP32 DEVKIT V1` (ou `NodeMCU-32S`)
- **Baudrate Serial**: `115200 bps`
- **Resolução ADC**: `12 bits` (`0` – `4095`)
