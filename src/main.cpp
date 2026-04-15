#include <cstdio>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/pulse_cnt.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>
#include <esp_timer.h>

// Servo configuration
#define SERVO_PIN 18
#define SERVO_CHANNEL LEDC_CHANNEL_0
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_FREQ 50  // 50 Гц для сервомотора
#define SERVO_RESOLUTION LEDC_TIMER_13_BIT  // 13 біт = 8192 рівня

// Константи для конвертації кута в мікросекунди
#define MIN_PULSE_US 500   // 0 градусів
#define MAX_PULSE_US 2500   // 180 градусів
#define PERIOD_US 20000     // 50 Гц = 20 мс період

// Verified Safe Pins for S3 N16R8
// Channel A and Channel B appeared to be swaped on the pinout of the encoder,
// so I swapped them in the code to match the expected behavior: increment when CW, decrement when CCW.
#define ENCODER_A_INPUT         5   // CLK
#define ENCODER_B_INPUT         4   // DT
#define ENCODER_BUTTON_INPUT    6   // SW
#define ENCODER_DEBOUNCE_NS     1000
#define ENCODER_MAX_COUNT       32767
#define ENCODER_MIN_COUNT       -32768
#define PCNT_UNIT PCNT_UNIT_0

const int ENCODER_PPR = 1024;
const int SAMPLE_INTERVAL_MS = 100;
const float SERVO_MIN_ANGLE = 0.0f;
const float SERVO_MAX_ANGLE = 180.0f;
const float SERVO_CENTER_ANGLE = 90.0f;
const float SERVO_COUNTS_PER_DEGREE = 2.0f;
const float SERVO_DIRECTION = -1.0f; // set to 1.0f for normal, -1.0f to flip
const unsigned long BUTTON_LONG_PRESS_MS = 1000;

static const char *TAG = "servo_ledc";


// Function helper to get milliseconds in ESP-IDF
unsigned long get_millis() {
    return (unsigned long)(esp_timer_get_time() / 1000);
}

static float clamp_angle(float angle)
    {
        if (angle < SERVO_MIN_ANGLE) {
            return SERVO_MIN_ANGLE;
        }
        if (angle > SERVO_MAX_ANGLE) {
            return SERVO_MAX_ANGLE;
        }
        return angle;
    }

static uint32_t angle_to_duty(float angle) {
    angle = clamp_angle(angle);
    float pulse_us = MIN_PULSE_US + (angle / SERVO_MAX_ANGLE) * (MAX_PULSE_US - MIN_PULSE_US);
    uint32_t max_duty = (1U << SERVO_RESOLUTION) - 1U;
    return (uint32_t)((pulse_us / PERIOD_US) * (float)max_duty);
}

void setup_servo_ledc() {
    ESP_ERROR_CHECK(gpio_reset_pin((gpio_num_t)SERVO_PIN));

    // Конфігурація таймера LEDC
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = SERVO_RESOLUTION;  // 13 біт
    timer_conf.timer_num = SERVO_TIMER;
    timer_conf.freq_hz = SERVO_FREQ;                // 50 Гц
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // Конфігурація каналу LEDC
    ledc_channel_config_t channel_conf = {};
    channel_conf.gpio_num = SERVO_PIN;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = SERVO_CHANNEL;
    channel_conf.intr_type = LEDC_INTR_DISABLE;
    channel_conf.timer_sel = SERVO_TIMER;
    channel_conf.duty = angle_to_duty(90.0f);  // Початкова позиція (90°)
    channel_conf.hpoint = 0;
    channel_conf.flags.output_invert = 0;

    esp_err_t channel_err = ledc_channel_config(&channel_conf);
    if (channel_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to bind LEDC to GPIO%d (err=0x%x). Pin may already be used.", SERVO_PIN, (unsigned int)channel_err);
        ESP_ERROR_CHECK(channel_err);
    }
}

void set_servo_angle_ledc(float angle) {
    // Обмежити кут від 0 до 180
    angle = clamp_angle(angle);
    float pulse_us = MIN_PULSE_US + (angle / SERVO_MAX_ANGLE) * (MAX_PULSE_US - MIN_PULSE_US);
    uint32_t duty = angle_to_duty(angle);

    // Записати в регістр LEDC
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL));

    ESP_LOGI(TAG, "Angle: %.1f -> Pulse: %.1f us -> Duty: %lu", angle, pulse_us, (unsigned long)duty);
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
    setup_servo_ledc();

    float current_servo_angle = SERVO_CENTER_ANGLE;
    float servo_step_scale = 1.0f;
    set_servo_angle_ledc(current_servo_angle);
    int last_servo_count = encoder.get_count();

    bool last_btn = false;
    bool long_press_handled = false;
    unsigned long button_press_start = 0;


    while (true) {
        // Keep calculating RPM frequently for accuracy
        encoder.calculate_rpm();
        encoder.detect_direction();

        int current_count = encoder.get_count();
        int delta_count = current_count - last_servo_count;
        if (delta_count != 0) {
            float target_servo_angle = current_servo_angle + (SERVO_DIRECTION * (((float)delta_count / SERVO_COUNTS_PER_DEGREE) * servo_step_scale));
            target_servo_angle = clamp_angle(target_servo_angle);
            set_servo_angle_ledc(target_servo_angle);
            current_servo_angle = target_servo_angle;
            last_servo_count = current_count;
        }

        bool current_btn = encoder.is_button_pressed();
        if (current_btn && !last_btn) {
            button_press_start = get_millis();
            long_press_handled = false;
        }

        if (current_btn && !long_press_handled) {
            unsigned long press_time = get_millis() - button_press_start;
            if (press_time >= BUTTON_LONG_PRESS_MS) {
                servo_step_scale = 1.0f;
                encoder.reset();
                current_servo_angle = SERVO_CENTER_ANGLE;
                set_servo_angle_ledc(current_servo_angle);
                last_servo_count = 0;
                long_press_handled = true;
                ESP_LOGI("SYSTEM", "Long press: step scale reset to %.2f and servo centered", servo_step_scale);
            }
        }

        if (!current_btn && last_btn) {
            if (!long_press_handled) {
                servo_step_scale *= 0.5f;
                ESP_LOGI("SYSTEM", "Short press: servo angle per tick halved. New scale: %.5f", servo_step_scale);
            }
        }
        last_btn = current_btn;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}