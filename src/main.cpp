#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

// LED (PWM)
#define LED_GPIO              GPIO_NUM_18  // GPIO18 = ADC1_CH17 — used as PWM output
#define LED_PWM_CHANNEL       LEDC_CHANNEL_0
#define LED_LEDC_TIMER        LEDC_TIMER_0
#define LED_LEDC_FREQUENCY    1000            // 1 kHz
#define LED_LEDC_RESOLUTION   LEDC_TIMER_10_BIT  // 10-bit: duty 0–1023

// Potentiometer (ADC)
#define POT_GPIO              GPIO_NUM_4
#define POT_ADC_UNIT          ADC_UNIT_1
#define POT_ADC_CHANNEL       ADC_CHANNEL_3   // GPIO4 = ADC1_CH3

// DC motor (PWM)
#define MOTOR_GPIO            GPIO_NUM_5   // GPIO5 = ADC1_CH5 — used as PWM output
#define MOTOR_PWM_CHANNEL     LEDC_CHANNEL_1
#define MOTOR_LEDC_TIMER      LEDC_TIMER_1
#define MOTOR_LEDC_FREQUENCY  20000    // 20 kHz для мотора
#define MOTOR_LEDC_RESOLUTION LEDC_TIMER_10_BIT  // 10-bit: duty 0–1023



static const char *TAG = "MAIN";

extern "C" void app_main(void) {

    // PWM setup for LED
    ledc_timer_config_t led_timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LED_LEDC_RESOLUTION,
        .timer_num = LED_LEDC_TIMER,
        .freq_hz = LED_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };
    ledc_timer_config(&led_timer_config);

    ledc_channel_config_t led_channel_config = {
        .gpio_num = LED_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LED_PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LED_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {
            .output_invert = 0,
        },
    };
    ledc_channel_config(&led_channel_config);

    // ADC setup (potentiometer on GPIO4 / ADC1_CH3)
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t adc_unit_cfg = {};
    adc_unit_cfg.unit_id = POT_ADC_UNIT;
    adc_unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t adc_chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,        // full 0–3.3 V range
        .bitwidth = ADC_BITWIDTH_DEFAULT,   // 12-bit: 0–4095
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, POT_ADC_CHANNEL, &adc_chan_cfg));

    // ADC calibration (curve fitting)
    adc_cali_handle_t cali_handle = NULL;
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = POT_ADC_UNIT,
        .chan     = POT_ADC_CHANNEL,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle));

    //DC motor PWM setup
    ledc_timer_config_t motor_timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = MOTOR_LEDC_RESOLUTION,
        .timer_num = MOTOR_LEDC_TIMER,
        .freq_hz = MOTOR_LEDC_FREQUENCY,      // 20 kHz для мотора
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };
    ledc_timer_config(&motor_timer_config);

    ledc_channel_config_t motor_channel_config = {
        .gpio_num = MOTOR_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = MOTOR_PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MOTOR_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {
            .output_invert = 0,
        },
    };
    ledc_channel_config(&motor_channel_config);

    while (1) {
        // Read potentiometer
        int raw = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, POT_ADC_CHANNEL, &raw));

        int voltage_mv = 0;
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv));

        // Map 12-bit ADC (0–4095) → 10-bit PWM duty (0–1023)
        int duty = (uint32_t)raw >> 2; // Equivalent to raw / 4, but faster (bit shift)

        // Control LED
        if (duty < 10) {
            duty = 0;
        }
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LED_PWM_CHANNEL, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LED_PWM_CHANNEL);

        // Control motor (same potentiometer, independent)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_PWM_CHANNEL);

        ESP_LOGI(TAG, "Raw: %4d | Voltage: %4d mV | Duty: %4d", raw, voltage_mv, duty);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
