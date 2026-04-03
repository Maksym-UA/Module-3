#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

#define LED_GPIO (gpio_num_t)18
#define THRESHOLD_LOW  1400  // Напруга для ввімкнення (Dark)
#define THRESHOLD_HIGH 1800  // Напруга для вимкнення (Light)
#define WINDOW_SIZE  10    // Розмір вікна фільтрації (чим більше, тим плавніша реакція)

// Функція Simple Moving Average
int get_sma_voltage(int new_sample) {
    static int samples[WINDOW_SIZE] = {0};
    static int index = 0;
    static long sum = 0;
    static bool filled = false;

    // Віднімаємо найстаріше значення, додаємо нове
    sum -= samples[index];
    samples[index] = new_sample;
    sum += samples[index];

    index = (index + 1) % WINDOW_SIZE;
    if (index == 0) filled = true;

    return (int)(sum / (filled ? WINDOW_SIZE : (index == 0 ? 1 : index)));
}

extern "C" void app_main(void) {

  // 1. Налаштування LED
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_GPIO, 0);

  // 2. Конфігурація модуля (Unit)
  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t adc1_config = {};
  adc1_config.unit_id = ADC_UNIT_1; // Використовуємо ADC1
  adc1_config.ulp_mode = ADC_ULP_MODE_DISABLE; // Вимикаємо режим ультранизького енергоспоживання

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_config, &adc1_handle));

  // 3. Налаштування каналу (LDR на GPIO 4)
  adc_oneshot_chan_cfg_t config = {
    .atten = ADC_ATTEN_DB_12,        // До 3.3V для S3
    .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));

  // 4. Налаштування калібрування для отримання мВ
  adc_cali_handle_t cali_handle = NULL;

  adc_cali_curve_fitting_config_t cali_config = {
    .unit_id = ADC_UNIT_1,
    .chan = ADC_CHANNEL_3,
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_DEFAULT,
  };

  // Створюємо схему калібрування за допомогою методу curve fitting
  ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));

  while (1) {
        int raw_val, voltage_mv;

        // 1. Зчитування
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &raw_val));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_val, &voltage_mv));

        // 2. Фільтрація (SMA)
        int filtered_voltage = get_sma_voltage(voltage_mv);

        // 3. Логіка з гістерезисом (опційно для стабільності)
        if (filtered_voltage < THRESHOLD_LOW) {
            gpio_set_level(LED_GPIO, 1);
        } else if (filtered_voltage > THRESHOLD_HIGH) {
            gpio_set_level(LED_GPIO, 0);
        }

        ESP_LOGI("ADC", "Voltage: %d mV | Filtered: %d mV", voltage_mv, filtered_voltage);

        vTaskDelay(pdMS_TO_TICKS(100)); // Час вибірки 100мс
    }
}