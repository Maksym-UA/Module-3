#include "encoder.hpp"

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_err.h>


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

// QuadratureEncoder class manages a quadrature encoder connected to the
// ESP32's pulse counter (PCNT) peripheral.
esp_err_t QuadratureEncoder::setup() {
    esp_err_t err;

    pcnt_unit_config_t unit_config = {
        .low_limit = ENCODER_MIN_COUNT,
        .high_limit = ENCODER_MAX_COUNT,
        .intr_priority = 0,
        .flags = {.accum_count = true},
    };
    err = pcnt_new_unit(&unit_config, &unit_);
    if (err != ESP_OK) return err;

    pcnt_glitch_filter_config_t filter_config = {.max_glitch_ns = ENCODER_DEBOUNCE_NS};
    err = pcnt_unit_set_glitch_filter(unit_, &filter_config);
    if (err != ESP_OK) return err;

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

    err = pcnt_new_channel(unit_, &config_a, &chan_a_);
    if (err != ESP_OK) return err;

    err = pcnt_new_channel(unit_, &config_b, &chan_b_);
    if (err != ESP_OK) return err;

    err = pcnt_channel_set_edge_action(
        chan_a_, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    if (err != ESP_OK) return err;

    err = pcnt_channel_set_level_action(
        chan_a_, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    if (err != ESP_OK) return err;

    err = pcnt_channel_set_edge_action(
        chan_b_, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    if (err != ESP_OK) return err;

    err = pcnt_channel_set_level_action(
        chan_b_, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    if (err != ESP_OK) return err;

    err = pcnt_unit_add_watch_point(unit_, ENCODER_PPR);
    if (err != ESP_OK) return err;

    err = pcnt_unit_add_watch_point(unit_, -ENCODER_PPR);
    if (err != ESP_OK) return err;

    pcnt_event_callbacks_t callbacks = {
        .on_reach = onFullRotation,
    };
    err = pcnt_unit_register_event_callbacks(unit_, &callbacks, this);
    if (err != ESP_OK) return err;

    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << ENCODER_BUTTON_INPUT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&btn_conf);
    if (err != ESP_OK) return err;

    err = pcnt_unit_enable(unit_);
    if (err != ESP_OK) return err;

    err = pcnt_unit_clear_count(unit_);
    if (err != ESP_OK) return err;

    err = pcnt_unit_start(unit_);
    if (err != ESP_OK) return err;

    unsigned long now = get_millis();
    last_direction_time_ = now;
    last_rpm_time_ = now;
    return ESP_OK;
}


esp_err_t QuadratureEncoder::getCount(int *count) const {
    if (count == nullptr) return ESP_ERR_INVALID_ARG;
    if (unit_ == nullptr) return ESP_ERR_INVALID_STATE;
    return pcnt_unit_get_count(unit_, count);
}

int QuadratureEncoder::getCount() const {
    int count = 0;
    if (getCount(&count) != ESP_OK) return 0;
    return count;
}

esp_err_t QuadratureEncoder::reset() {
    if (unit_ == nullptr) return ESP_ERR_INVALID_STATE;
    return pcnt_unit_clear_count(unit_);
}


// Direction detection is based on counting the number of pulses in each direction 
// over a short time window.
void QuadratureEncoder::detectDirection() {
    int current_position = getCount();
    unsigned long now = get_millis();

    if ((now - last_direction_time_) < SAMPLE_INTERVAL_MS) {
        return;
    }

    int delta = current_position - last_position_;
    if (delta > 0) {
        last_direction_state_ = 1;
    } else if (delta < 0) {
        last_direction_state_ = -1;
    } else {
        last_direction_state_ = 0;
    }

    last_position_ = current_position;
    last_direction_time_ = now;
}


void QuadratureEncoder::calculateRpm() {
    int current_count = getCount();
    unsigned long now = get_millis();
    unsigned long elapsed_ms = now - last_rpm_time_;

    if (elapsed_ms < SAMPLE_INTERVAL_MS) {
        return;
    }

    int delta_counts = current_count - last_count_;
    float rpm = 0.0f;
    if (elapsed_ms > 0) {
        rpm = ((float)delta_counts * 60000.0f) / ((float)ENCODER_PPR * (float)elapsed_ms);
    }

    ESP_LOGI("encoder", "count=%d, delta=%d, dir=%d, rpm=%.2f", current_count, delta_counts, last_direction_state_, rpm);

    last_count_ = current_count;
    last_rpm_time_ = now;
}


bool QuadratureEncoder::isButtonPressed() const {
    int level = gpio_get_level((gpio_num_t)ENCODER_BUTTON_INPUT);
    return level == 0;
}


bool QuadratureEncoder::onFullRotation(
    pcnt_unit_handle_t,
    const pcnt_watch_event_data_t *edata,
    void *user_ctx) {
    if (edata == nullptr || user_ctx == nullptr) {
        return false;
    }

    auto *self = static_cast<QuadratureEncoder *>(user_ctx);
    if (edata->watch_point_value == ENCODER_PPR) {
        self->last_direction_state_ = 1;
    } else if (edata->watch_point_value == -ENCODER_PPR) {
        self->last_direction_state_ = -1;
    }

    return false;
}