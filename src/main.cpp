#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h" 

// Verified Safe Pins for S3 N16R8
//Channel A and Channel B appeared to be swaped on the pinout of the encoder,
//so I swapped them in the code to match the expected behavior: increment when CW, decrement when CCW.
#define ENCODER_A_INPUT         5   // CLK
#define ENCODER_B_INPUT         4   // DT
#define ENCODER_BUTTON_INPUT    6   // SW
#define ENCODER_DEBOUNCE_NS     1000
#define ENCODER_MAX_COUNT       32767
#define ENCODER_MIN_COUNT       -32768
#define PCNT_UNIT PCNT_UNIT_0

const int ENCODER_PPR = 1024;
const int SAMPLE_INTERVAL_MS = 100;

// Function helper to get milliseconds in ESP-IDF
unsigned long get_millis() {
    return (unsigned long)(esp_timer_get_time() / 1000);
}



class QuadratureEncoder {
private:
    pcnt_unit_handle_t unit = nullptr;
    pcnt_channel_handle_t chan_a = nullptr;
    pcnt_channel_handle_t chan_b = nullptr;
    const char* TAG = "ENCODER";

    int last_count = 0;
    int last_position = 0;
    int last_direction_state = 0; // -1: CCW, 0: STATIONARY, 1: CW
    unsigned long last_direction_time = 0;
    unsigned long last_rpm_time = 0;

    static bool on_full_rotation(pcnt_unit_handle_t unit,
                                 const pcnt_watch_event_data_t *edata,
                                 void *user_ctx) {
        if (edata->watch_point_value == ENCODER_PPR) {
            ESP_LOGI("ENCODER", "Full Rotation CW!");
        } else if (edata->watch_point_value == -ENCODER_PPR) {
            ESP_LOGI("ENCODER", "Full Rotation CCW!");
        }
        return false;
    }

public:
    QuadratureEncoder() {
        pcnt_unit_config_t unit_config = {
            .low_limit = ENCODER_MIN_COUNT,
            .high_limit = ENCODER_MAX_COUNT,
            .intr_priority = 0,
            .flags = { .accum_count = true }
        };
        ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &unit));

        pcnt_glitch_filter_config_t filter_config = { .max_glitch_ns = ENCODER_DEBOUNCE_NS };
        ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(unit, &filter_config));

        pcnt_chan_config_t config_a = {
            .edge_gpio_num = ENCODER_A_INPUT,
            .level_gpio_num = ENCODER_B_INPUT, .flags = {} };
        pcnt_chan_config_t config_b = {
            .edge_gpio_num = ENCODER_B_INPUT,
            .level_gpio_num = ENCODER_A_INPUT, .flags = {} };
        ESP_ERROR_CHECK(pcnt_new_channel(unit, &config_a, &chan_a));
        ESP_ERROR_CHECK(pcnt_new_channel(unit, &config_b, &chan_b));

        // Edge/Level Actions (Standard X4 Logic)
        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
            chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(
            chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
            chan_b, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(
            chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

        // Watch points: report full-rotation events in both directions
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(unit, ENCODER_PPR));
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(unit, -ENCODER_PPR));

        pcnt_event_callbacks_t callbacks = {
            .on_reach = on_full_rotation,
        };
        ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(unit, &callbacks, this));

        // Button Setup
        gpio_config_t btn_conf = {
            .pin_bit_mask = (1ULL << ENCODER_BUTTON_INPUT),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_ERROR_CHECK(gpio_config(&btn_conf));

        ESP_ERROR_CHECK(pcnt_unit_enable(unit));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(unit));
        ESP_ERROR_CHECK(pcnt_unit_start(unit));

        unsigned long now = get_millis();
        last_direction_time = now;
        last_rpm_time = now;
    }

    void detect_direction() {
        int current_position = get_count();
        unsigned long current_time = get_millis();
        unsigned long time_delta = current_time - last_direction_time;

        if (time_delta >= SAMPLE_INTERVAL_MS) {
            int delta = current_position - last_position;
            int direction_state = 0;

            if (delta > 0) {
                direction_state = 1;
            } else if (delta < 0) {
                direction_state = -1;
            }

            if (direction_state != last_direction_state) {
                if (direction_state > 0) {
                    ESP_LOGI("ENCODER", "Rotating CLOCKWISE");
                } else if (direction_state < 0) {
                    ESP_LOGI("ENCODER", "Rotating COUNTER-CLOCKWISE");
                } else {
                    ESP_LOGI("ENCODER", "STATIONARY");
                }
                last_direction_state = direction_state;
            }

            last_position = current_position;
            last_direction_time = current_time;
        }
    }

    void calculate_rpm() {
        int current_count = get_count();
        unsigned long current_time = get_millis();
        unsigned long time_delta = current_time - last_rpm_time;

        if (time_delta >= SAMPLE_INTERVAL_MS) {
            int count_delta = current_count - last_count;
            // RPM = (delta_steps / PPR) / (delta_ms / 60000)
            float rpm = ((float)count_delta / ENCODER_PPR) * (60000.0f / (float)time_delta);

            ESP_LOGI("ENCODER", "RPM: %.2f | Raw: %d", rpm, current_count);

            last_count = current_count;
            last_rpm_time = current_time;
        }
    }

    int get_count() const {
        int count = 0;
        pcnt_unit_get_count(unit, &count);
        return count;
    }

    bool is_button_pressed() const {
        return gpio_get_level((gpio_num_t)ENCODER_BUTTON_INPUT) == 0;
    }

    void reset() {
        pcnt_unit_clear_count(unit);
    }
};

extern "C" void app_main() {
    QuadratureEncoder encoder;
    bool last_btn = false;


    while (true) {
        // Keep calculating RPM frequently for accuracy
        encoder.calculate_rpm();
        encoder.detect_direction();

        bool current_btn = encoder.is_button_pressed();
        if (current_btn && !last_btn) {
            encoder.reset();
            ESP_LOGI("SYSTEM", "Counter Reset");
        }
        last_btn = current_btn;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}