#include "encoder.hpp"

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>

namespace {
constexpr int ENCODER_A_INPUT = 5;
constexpr int ENCODER_B_INPUT = 4;
constexpr int ENCODER_BUTTON_INPUT = 6;
constexpr int ENCODER_DEBOUNCE_NS = 1000;
constexpr int ENCODER_MAX_COUNT = 32767;
constexpr int ENCODER_MIN_COUNT = -32768;

constexpr int ENCODER_PPR = 1024;
constexpr int SAMPLE_INTERVAL_MS = 100;

unsigned long get_millis() {
    return (unsigned long)(esp_timer_get_time() / 1000);
}
}

bool QuadratureEncoder::onFullRotation(pcnt_unit_handle_t,
                                       const pcnt_watch_event_data_t *edata,
                                       void *) {
    if (edata->watch_point_value == ENCODER_PPR) {
        ESP_LOGI("ENCODER", "Full Rotation CW!");
    } else if (edata->watch_point_value == -ENCODER_PPR) {
        ESP_LOGI("ENCODER", "Full Rotation CCW!");
    }
    return false;
}

QuadratureEncoder::QuadratureEncoder() {
    pcnt_unit_config_t unit_config = {
        .low_limit = ENCODER_MIN_COUNT,
        .high_limit = ENCODER_MAX_COUNT,
        .intr_priority = 0,
        .flags = {.accum_count = true},
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &unit_));

    pcnt_glitch_filter_config_t filter_config = {.max_glitch_ns = ENCODER_DEBOUNCE_NS};
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(unit_, &filter_config));

    pcnt_chan_config_t config_a = {
        .edge_gpio_num = ENCODER_A_INPUT,
        .level_gpio_num = ENCODER_B_INPUT,
        .flags = {},
    };
    pcnt_chan_config_t config_b = {
        .edge_gpio_num = ENCODER_B_INPUT,
        .level_gpio_num = ENCODER_A_INPUT,
        .flags = {},
    };

    ESP_ERROR_CHECK(pcnt_new_channel(unit_, &config_a, &chan_a_));
    ESP_ERROR_CHECK(pcnt_new_channel(unit_, &config_b, &chan_b_));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        chan_a_, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        chan_a_, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        chan_b_, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        chan_b_, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(unit_, ENCODER_PPR));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(unit_, -ENCODER_PPR));

    pcnt_event_callbacks_t callbacks = {
        .on_reach = onFullRotation,
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(unit_, &callbacks, this));

    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << ENCODER_BUTTON_INPUT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_conf));

    ESP_ERROR_CHECK(pcnt_unit_enable(unit_));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(unit_));
    ESP_ERROR_CHECK(pcnt_unit_start(unit_));

    unsigned long now = get_millis();
    last_direction_time_ = now;
    last_rpm_time_ = now;
}

void QuadratureEncoder::detectDirection() {
    int current_position = getCount();
    unsigned long current_time = get_millis();
    unsigned long time_delta = current_time - last_direction_time_;

    if (time_delta >= SAMPLE_INTERVAL_MS) {
        int delta = current_position - last_position_;
        int direction_state = 0;

        if (delta > 0) {
            direction_state = 1;
        } else if (delta < 0) {
            direction_state = -1;
        }

        if (direction_state != last_direction_state_) {
            if (direction_state > 0) {
                ESP_LOGI("ENCODER", "Rotating CLOCKWISE");
            } else if (direction_state < 0) {
                ESP_LOGI("ENCODER", "Rotating COUNTER-CLOCKWISE");
            } else {
                ESP_LOGI("ENCODER", "STATIONARY");
            }
            last_direction_state_ = direction_state;
        }

        last_position_ = current_position;
        last_direction_time_ = current_time;
    }
}

void QuadratureEncoder::calculateRpm() {
    int current_count = getCount();
    unsigned long current_time = get_millis();
    unsigned long time_delta = current_time - last_rpm_time_;

    if (time_delta >= SAMPLE_INTERVAL_MS) {
        int count_delta = current_count - last_count_;
        float rpm = ((float)count_delta / ENCODER_PPR) * (60000.0f / (float)time_delta);

        ESP_LOGI("ENCODER", "RPM: %.2f | Raw: %d", rpm, current_count);

        last_count_ = current_count;
        last_rpm_time_ = current_time;
    }
}

int QuadratureEncoder::getCount() const {
    int count = 0;
    pcnt_unit_get_count(unit_, &count);
    return count;
}

bool QuadratureEncoder::isButtonPressed() const {
    return gpio_get_level((gpio_num_t)ENCODER_BUTTON_INPUT) == 0;
}

void QuadratureEncoder::reset() {
    pcnt_unit_clear_count(unit_);
}
