#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Configuration Defines
#define ENCODER_A_INPUT         16
#define ENCODER_B_INPUT         17
#define ENCODER_BUTTON_INPUT    15
#define ENCODER_DEBOUNCE_NS     1000
#define ENCODER_MAX_COUNT       32767
#define ENCODER_MIN_COUNT       -32768

class QuadratureEncoder {
private:
    pcnt_unit_handle_t unit = nullptr;
    pcnt_channel_handle_t chan_a = nullptr;
    pcnt_channel_handle_t chan_b = nullptr;
    const char* TAG = "ENCODER";

public:
    QuadratureEncoder() {
        // 1. Initialize Pulse Counter Unit
        pcnt_unit_config_t unit_config = {
            .low_limit = ENCODER_MIN_COUNT,
            .high_limit = ENCODER_MAX_COUNT,
            .intr_priority = 0,
            .flags = { .accum_count = true }
        };
        ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &unit));

        // 2. Configure Glitch Filter with specified debounce time
        pcnt_glitch_filter_config_t filter_config = {
            .max_glitch_ns = ENCODER_DEBOUNCE_NS
        };
        ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(unit, &filter_config));

        // 3. Setup Channels for X4 decoding
        pcnt_chan_config_t config_a = {
            .edge_gpio_num = ENCODER_A_INPUT,
            .level_gpio_num = ENCODER_B_INPUT,
            .flags = {}
        };
        pcnt_chan_config_t config_b = {
            .edge_gpio_num = ENCODER_B_INPUT,
            .level_gpio_num = ENCODER_A_INPUT,
            .flags = {}
        };

        ESP_ERROR_CHECK(pcnt_new_channel(unit, &config_a, &chan_a));
        ESP_ERROR_CHECK(pcnt_new_channel(unit, &config_b, &chan_b));

        // Edge/Level Actions (Standard X4 Logic)
        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

        // 4. Initialize Button GPIO
        gpio_config_t btn_conf = {
            .pin_bit_mask = (1ULL << ENCODER_BUTTON_INPUT),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_ERROR_CHECK(gpio_config(&btn_conf));

        // 5. Start Unit
        ESP_ERROR_CHECK(pcnt_unit_enable(unit));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(unit));
        ESP_ERROR_CHECK(pcnt_unit_start(unit));

        ESP_LOGI(TAG, "Encoder (Pins %d,%d) and Button (%d) Ready",
                 ENCODER_A_INPUT, ENCODER_B_INPUT, ENCODER_BUTTON_INPUT);
    }

    ~QuadratureEncoder() {
        if (unit) {
            pcnt_unit_stop(unit);
            pcnt_unit_disable(unit);
            pcnt_del_unit(unit);
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
    bool last_button_state = false;

    while (true) {
        bool current_button_pressed = encoder.is_button_pressed();
        int pos = encoder.get_count();

        // Reset logic: Triggered when button transitions from NOT pressed to pressed
        if (current_button_pressed && !last_button_state) {
            encoder.reset();
            ESP_LOGI("ENCODER", "Button Pressed: Counter Reset to 0");
        }

        // Update state for next iteration
        last_button_state = current_button_pressed;

        // Print current status
        printf("Pos: %d | Button: %s\n", pos, current_button_pressed ? "DOWN" : "UP");

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}