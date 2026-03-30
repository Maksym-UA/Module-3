#include <Arduino.h>
#include "esp_adc_cal.h" // Include the ESP32 ADC calibration library

// Configure parameters
struct Config {
  static constexpr uint8_t ADC_PIN = 4; //ADC1_CH3
  static constexpr uint32_t BAUDRATE = 115200;
  static constexpr float VCC = 3.3;         // Напруга живлення
  static constexpr int ADC_RES = 4095;      // 12-біт розрядність
  static esp_adc_cal_characteristics_t adc_chars;
};

esp_adc_cal_characteristics_t Config::adc_chars;

void setup() {

  Serial.begin(Config::BAUDRATE);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, Config::VCC, &Config::adc_chars);

  Serial.println("RAW \t Calc_V(mV) \t Calib_V(mV) \t Error(%)");
  Serial.println("-------------------------------------------------------");
}

void loop() {
  // RAW value from ADC
  int rawValue = analogRead(Config::ADC_PIN);

  float voltageCalc = (rawValue / (float)Config::ADC_RES) * Config::VCC * 1000.0; // Перевод в мВ

  uint32_t voltageCalib = esp_adc_cal_raw_to_voltage(rawValue, &Config::adc_chars);

  float errorPercent = 0;
  if (voltageCalib > 0) {
    errorPercent = (abs(voltageCalc - (float)voltageCalib) / (float)voltageCalib) * 100.0;
  }

  // Вивід у табличному вигляді
  Serial.print(rawValue);
  Serial.print("\t ");
  Serial.print(voltageCalc, 1);
  Serial.print("\t\t ");
  Serial.print(voltageCalib);
  Serial.print("\t\t ");
  Serial.print(errorPercent, 2);
  Serial.println("%");


   delay(100); // Оновлення кожні 100 мс

}