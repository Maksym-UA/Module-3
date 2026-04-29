#include "buzzer.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/ledc.h>


namespace {
    constexpr int BUZZER_PIN = 17;
    constexpr ledc_channel_t BUZZER_CHANNEL = LEDC_CHANNEL_1;
    constexpr ledc_timer_t BUZZER_TIMER = LEDC_TIMER_1;
    constexpr int BUZZER_FREQ = 2000;
    constexpr ledc_timer_bit_t BUZZER_RESOLUTION = LEDC_TIMER_10_BIT;
    constexpr unsigned long LIMIT_BEEP_MS = 60;
    const char *TAG = "buzzer";
}


// Buzzer class manages a buzzer connected to the ESP32 using the LEDC peripheral.
esp_err_t Buzzer::setup() {
    esp_err_t err = gpio_reset_pin((gpio_num_t)BUZZER_PIN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_reset_pin failed: %s", esp_err_to_name(err));
        return err;
    }

    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = BUZZER_RESOLUTION;
    timer_conf.timer_num = BUZZER_TIMER;
    timer_conf.freq_hz = BUZZER_FREQ;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(err));
        return err;
    }

    ledc_channel_config_t channel_conf = {};
    channel_conf.gpio_num = BUZZER_PIN;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = BUZZER_CHANNEL;
    channel_conf.intr_type = LEDC_INTR_DISABLE;
    channel_conf.timer_sel = BUZZER_TIMER;
    channel_conf.duty = 0;
    channel_conf.hpoint = 0;
    channel_conf.flags.output_invert = 0;
    err = ledc_channel_config(&channel_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
}


esp_err_t Buzzer::playLimitBeep() {
    uint32_t duty = ((1U << BUZZER_RESOLUTION) - 1U) / 2U;
    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, duty);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_set_duty(on) failed: %s", esp_err_to_name(err));
        return err;
    }

    err = ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_update_duty(on) failed: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(LIMIT_BEEP_MS));

    err = ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_set_duty(off) failed: %s", esp_err_to_name(err));
        return err;
    }

    err = ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_update_duty(off) failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}
