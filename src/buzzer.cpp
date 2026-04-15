#include "buzzer.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/gpio.h>
#include <driver/ledc.h>

namespace {
constexpr int BUZZER_PIN = 17;
constexpr ledc_channel_t BUZZER_CHANNEL = LEDC_CHANNEL_1;
constexpr ledc_timer_t BUZZER_TIMER = LEDC_TIMER_1;
constexpr int BUZZER_FREQ = 2000;
constexpr ledc_timer_bit_t BUZZER_RESOLUTION = LEDC_TIMER_10_BIT;
constexpr unsigned long LIMIT_BEEP_MS = 60;
}

void Buzzer::setup() {
    ESP_ERROR_CHECK(gpio_reset_pin((gpio_num_t)BUZZER_PIN));

    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = BUZZER_RESOLUTION;
    timer_conf.timer_num = BUZZER_TIMER;
    timer_conf.freq_hz = BUZZER_FREQ;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {};
    channel_conf.gpio_num = BUZZER_PIN;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = BUZZER_CHANNEL;
    channel_conf.intr_type = LEDC_INTR_DISABLE;
    channel_conf.timer_sel = BUZZER_TIMER;
    channel_conf.duty = 0;
    channel_conf.hpoint = 0;
    channel_conf.flags.output_invert = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

void Buzzer::playLimitBeep() {
    uint32_t duty = ((1U << BUZZER_RESOLUTION) - 1U) / 2U;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL));
    vTaskDelay(pdMS_TO_TICKS(LIMIT_BEEP_MS));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL));
}
