#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include "esp_err.h"

// Константи для сервомотора
#define SERVO_PIN               GPIO_NUM_18
#define SERVO_CHANNEL LEDC_CHANNEL_0
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_FREQ 50  // 50 Гц для сервомотора
#define SERVO_RESOLUTION LEDC_TIMER_13_BIT  // 13 біт = 8192 рівня


// Потенціометр підключений до GPIO4 (ADC1_CH3)
#define POT_GPIO              GPIO_NUM_4
#define POT_ADC_UNIT          ADC_UNIT_1
#define POT_ADC_CHANNEL       ADC_CHANNEL_3   // GPIO4 = ADC1_CH3

// Константи для конвертації кута в мікросекунди
#define MIN_PULSE_US 500   // 0 градусів
#define MAX_PULSE_US 2500   // 180 градусів
#define PERIOD_US 20000     // 50 Гц = 20 мс період

static const char *TAG = "servo_ledc";
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
static int adc_min_value = 0;      // Мінімум потенціометра
static int adc_max_value = 4095;   // Максимум потенціометра

/**
 * РЕЖИМ КАЛІБРУВАННЯ: щоб знайти реальний діапазон потенціометра
 * 1. Поверніть потенціометр до лівого крайнього положення, запишіть значення з логу
 * 2. Поверніть потенціометр до правого крайнього положення, запишіть значення з логу
 * 3. Замініть adc_min_value та adc_max_value на ці значення
 * 4. Перезавантажте прошивку
 * Приклад: якщо ліво=500, право=3500, то:
 *   static int adc_min_value = 500;
 *   static int adc_max_value = 3500;
 */

static float clamp_angle(float angle)
    {
        if (angle < 0.0f) {
            return 0.0f;
        }
        if (angle > 180.0f) {
            return 180.0f;
        }
        return angle;
    }

//esp_err_t, is important: it communicates success (ESP_OK) or the exact failure code
// from the underlying driver calls. That makes startup robust, because app_main() can
// stop early and log a meaningful error instead of continuing with a partially configured servo.
esp_err_t setup_potentiometer_ledc() {
    // Конфігурація ADC для потенціометра
    adc_oneshot_unit_init_cfg_t adc_unit_cfg = {};
    adc_unit_cfg.unit_id = POT_ADC_UNIT;
    adc_unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    esp_err_t err = adc_oneshot_new_unit(&adc_unit_cfg, &adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(err));
        return err;
    }

    adc_oneshot_chan_cfg_t adc_chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,        // full 0–3.3 V range
        .bitwidth = ADC_BITWIDTH_DEFAULT,   // 12-bit: 0–4095
    };
    err = adc_oneshot_config_channel(adc_handle, POT_ADC_CHANNEL, &adc_chan_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(err));
        return err;
    }

    // ADC calibration (curve fitting)
    cali_handle = NULL;
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = POT_ADC_UNIT,
        .chan     = POT_ADC_CHANNEL,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_cali_create_scheme_curve_fitting failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}


esp_err_t setup_servo_ledc() {
    // Конфігурація таймера LEDC
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = SERVO_RESOLUTION;  // 13 біт
    timer_conf.timer_num = SERVO_TIMER;
    timer_conf.freq_hz = SERVO_FREQ;                // 50 Гц
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(err));
        return err;
    }

    // Конфігурація каналу LEDC
    ledc_channel_config_t channel_conf = {};
    channel_conf.gpio_num = SERVO_PIN;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = SERVO_CHANNEL;
    channel_conf.intr_type = LEDC_INTR_DISABLE;
    channel_conf.timer_sel = SERVO_TIMER;
    channel_conf.duty = 614;  // Початкова позиція (90°)
    channel_conf.hpoint = 0;
    channel_conf.flags.output_invert = 0;
    err = ledc_channel_config(&channel_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}


esp_err_t set_servo_angle_ledc(float angle) {
    // Обмежити кут від 0 до 180
    angle = clamp_angle(angle);

    // Конвертувати кут в мікросекунди
    float pulse_us = MIN_PULSE_US + (angle / 180.0) * (MAX_PULSE_US - MIN_PULSE_US);

    // Конвертувати мікросекунди в значення duty
    // duty = (pulse_us / PERIOD_US) * 8192
    uint32_t duty = (uint32_t)((pulse_us / PERIOD_US) * (1 << SERVO_RESOLUTION));

    // Записати в регістр LEDC
    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_set_duty failed: %s", esp_err_to_name(err));
        return err;
    }

    err = ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_update_duty failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Angle: %.1f -> Pulse: %.1f us -> Duty: %lu", angle, pulse_us, (unsigned long)duty);
    return ESP_OK;
}


extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting servo LEDC control");
    esp_err_t err = setup_potentiometer_ledc();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "setup_potentiometer_ledc failed: %s", esp_err_to_name(err));
        return;
    }
    err = setup_servo_ledc();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "setup_servo_ledc failed: %s", esp_err_to_name(err));
        return;
    }
    err = set_servo_angle_ledc(90);  // Початкова позиція
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_servo_angle_ledc failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Servo LEDC control initialized");


    while (1) {
        // Прочитати значення потенціометра (raw ADC)
        int potValue = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, POT_ADC_CHANNEL, &potValue));

        // Калібрування: конвертувати raw ADC в напругу (mV)
        int voltage_mv = 0;
        if (cali_handle != NULL) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, potValue, &voltage_mv));
        }

        // Обмежити до реального діапазону потенціометра
        if (potValue < adc_min_value) potValue = adc_min_value;
        if (potValue > adc_max_value) potValue = adc_max_value;

        // Конвертувати значення потенціометра в кут (0-180 градусів) з використанням реального діапазону
        float angle = ((float)(potValue - adc_min_value) / (adc_max_value - adc_min_value)) * 180.0;

        // Встановити кут сервомотора
        err = set_servo_angle_ledc(angle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "set_servo_angle_ledc failed in loop: %s", esp_err_to_name(err));
        }

        // Логування для налагодження
        //ESP_LOGI(TAG, "Pot: raw=%d, voltage=%d mV, angle=%.1f° | (min=%d, max=%d)",
                // potValue, voltage_mv, angle, adc_min_value, adc_max_value);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
