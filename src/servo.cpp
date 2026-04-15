#include "servo.hpp"

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>

namespace {
constexpr int SERVO_PIN = 18;
constexpr ledc_channel_t SERVO_CHANNEL = LEDC_CHANNEL_0;
constexpr ledc_timer_t SERVO_TIMER = LEDC_TIMER_0;
constexpr int SERVO_FREQ = 50;
constexpr ledc_timer_bit_t SERVO_RESOLUTION = LEDC_TIMER_13_BIT;

constexpr float MIN_PULSE_US = 500.0f;
constexpr float MAX_PULSE_US = 2500.0f;
constexpr float PERIOD_US = 20000.0f;

const char *TAG = "servo_ledc";
}

float Servo::clampAngle(float angle) const {
    if (angle < kMinAngle) {
        return kMinAngle;
    }
    if (angle > kMaxAngle) {
        return kMaxAngle;
    }
    return angle;
}

uint32_t Servo::angleToDuty(float angle) {
    if (angle < kMinAngle) {
        angle = kMinAngle;
    }
    if (angle > kMaxAngle) {
        angle = kMaxAngle;
    }

    float pulse_us = MIN_PULSE_US + (angle / kMaxAngle) * (MAX_PULSE_US - MIN_PULSE_US);
    uint32_t max_duty = (1U << SERVO_RESOLUTION) - 1U;
    return (uint32_t)((pulse_us / PERIOD_US) * (float)max_duty);
}

void Servo::setup() {
    ESP_ERROR_CHECK(gpio_reset_pin((gpio_num_t)SERVO_PIN));

    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = SERVO_RESOLUTION;
    timer_conf.timer_num = SERVO_TIMER;
    timer_conf.freq_hz = SERVO_FREQ;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {};
    channel_conf.gpio_num = SERVO_PIN;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = SERVO_CHANNEL;
    channel_conf.intr_type = LEDC_INTR_DISABLE;
    channel_conf.timer_sel = SERVO_TIMER;
    channel_conf.duty = angleToDuty(kCenterAngle);
    channel_conf.hpoint = 0;
    channel_conf.flags.output_invert = 0;

    esp_err_t channel_err = ledc_channel_config(&channel_conf);
    if (channel_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to bind LEDC to GPIO%d (err=0x%x). Pin may already be used.", SERVO_PIN, (unsigned int)channel_err);
        ESP_ERROR_CHECK(channel_err);
    }
}

void Servo::setAngle(float angle) {
    angle = clampAngle(angle);
    float pulse_us = MIN_PULSE_US + (angle / kMaxAngle) * (MAX_PULSE_US - MIN_PULSE_US);
    uint32_t duty = angleToDuty(angle);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL));

    ESP_LOGI(TAG, "Angle: %.1f -> Pulse: %.1f us -> Duty: %lu", angle, pulse_us, (unsigned long)duty);
}
