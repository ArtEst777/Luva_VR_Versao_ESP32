/**
 * @file validacao_adc_sensores.ino
 * @brief Firmware de teste e validação elétrica de bancada para os canais analógicos (Placa Universal)
 * @project L-UVA - Luva de Realidade Virtual de Baixo Custo (L-VIS / UnB)
 *
 * Verifica a leitura dos quatro canais de sensores flexíveis com resolução de 12 bits (0 a 4095)
 * e atenuação de 11 dB no conversor analógico-digital (ADC) da ESP32 DevKit V1.
 *
 * Atenção: Não ativar Wi-Fi ou ESP-NOW durante este ensaio caso o pino GPIO25 (ADC2) esteja em uso.
 */

#include <Arduino.h>

const int PIN_INDICADOR = 36; // VP  - ADC1
const int PIN_MEDIO     = 34; // D34 - ADC1
const int PIN_ANELAR    = 32; // D32 - ADC1
const int PIN_MINDINHO  = 25; // D25 - ADC2

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);

    analogSetPinAttenuation(PIN_INDICADOR, ADC_11db);
    analogSetPinAttenuation(PIN_MEDIO,     ADC_11db);
    analogSetPinAttenuation(PIN_ANELAR,    ADC_11db);
    analogSetPinAttenuation(PIN_MINDINHO,  ADC_11db);
}

void loop() {
    Serial.print("Indicador: ");
    Serial.print(analogRead(PIN_INDICADOR));

    Serial.print(" | Medio: ");
    Serial.print(analogRead(PIN_MEDIO));

    Serial.print(" | Anelar: ");
    Serial.print(analogRead(PIN_ANELAR));

    Serial.print(" | Mindinho: ");
    Serial.println(analogRead(PIN_MINDINHO));

    delay(100);
}
