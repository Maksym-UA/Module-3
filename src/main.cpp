#include <Arduino.h>
#include "esp_adc_cal.h" // Include the ESP32 ADC calibration library

// Configure parameters
struct Config {
  static constexpr uint8_t ADC_PIN = 4; //ADC1_CH3
  static constexpr uint32_t BAUDRATE = 115200;
  static constexpr float VCC = 3.3; //volts
  static constexpr int ADC_RES_BITS = 12; // 12-bit resolution
  static constexpr int ADC_MAX = (1 << ADC_RES_BITS) - 1;
  static esp_adc_cal_characteristics_t adc_chars;
};

esp_adc_cal_characteristics_t Config::adc_chars;

void setup() {

  Serial.begin(Config::BAUDRATE);
  analogReadResolution(Config::ADC_RES_BITS); // Set ADC resolution to 12 bits
  analogSetAttenuation(ADC_11db);// Set attenuation to 11dB for a wider voltage range (up to ~3.6V)

  // Characterize ADC
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, Config::VCC, &Config::adc_chars);
}

void loop() {
  // RAW value from ADC
  int rawValue = analogRead(Config::ADC_PIN);

  // Convert to mV using the formula: (raw / max_raw) * VCC * 1000
  float voltageCalc = (rawValue / (float)Config::ADC_MAX) * Config::VCC * 1000.0; // Convert to mV

  // Get calibrated voltage in mV using the ESP32 ADC calibration function
  uint32_t voltageCalib = esp_adc_cal_raw_to_voltage(rawValue, &Config::adc_chars);

  float errorPercent = 0;
  if (voltageCalib > 0) {
    errorPercent = (abs(voltageCalc - (float)voltageCalib) / (float)voltageCalib) * 100.0;
  }

  Serial.print(rawValue);
  Serial.print("\t ");
  Serial.print(voltageCalc, 1);
  Serial.print("\t\t ");
  Serial.print(voltageCalib);
  Serial.print("\t\t ");
  Serial.print(errorPercent, 2);
  Serial.println("%");

  // Update every 100 ms
  delay(100);
}