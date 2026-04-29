#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_err.h>

#include "buzzer.hpp"
#include "encoder.hpp"
#include "servo.hpp"


const float SERVO_COUNTS_PER_DEGREE = 2.0f;
const float SERVO_DIRECTION = -1.0f;
const unsigned long BUTTON_LONG_PRESS_MS = 1000;
const unsigned long LIMIT_BEEP_COOLDOWN_MS = 120;
static const char *TAG = "app";


unsigned long get_millis() {
    return (unsigned long)(esp_timer_get_time() / 1000);
}


extern "C" void app_main() {
    Servo servo;
    Buzzer buzzer;
    QuadratureEncoder encoder;

    // Initialize peripherals and handle errors
    esp_err_t err = encoder.setup();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "encoder.setup failed: %s", esp_err_to_name(err));
        return;
    }

    err = servo.setup();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "servo.setup failed: %s", esp_err_to_name(err));
        return;
    }

    err = buzzer.setup();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "buzzer.setup failed: %s", esp_err_to_name(err));
        return;
    }

    float current_servo_angle = Servo::kCenterAngle;
    float servo_step_scale = 1.0f;
    err = servo.setAngle(current_servo_angle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "servo.setAngle(center) failed: %s", esp_err_to_name(err));
        return;
    }
    int last_servo_count = encoder.getCount();

    bool last_btn = false;
    bool long_press_handled = false;
    unsigned long button_press_start = 0;
    unsigned long last_limit_beep_time = 0;


    while (true) {
        encoder.calculateRpm();
        encoder.detectDirection();

        int current_count = encoder.getCount();
        int delta_count = current_count - last_servo_count;
        if (delta_count != 0) {
            float requested_servo_angle = current_servo_angle + (SERVO_DIRECTION * (((float)delta_count / SERVO_COUNTS_PER_DEGREE) * servo_step_scale));
            float target_servo_angle = servo.clampAngle(requested_servo_angle);

            if (target_servo_angle != requested_servo_angle) {
                unsigned long now = get_millis();
                if ((now - last_limit_beep_time) >= LIMIT_BEEP_COOLDOWN_MS) {
                    err = buzzer.playLimitBeep();
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "buzzer.playLimitBeep failed: %s", esp_err_to_name(err));
                    }
                    last_limit_beep_time = get_millis();
                }
            }

            if (target_servo_angle != current_servo_angle) {
                err = servo.setAngle(target_servo_angle);
                if (err == ESP_OK) {
                    current_servo_angle = target_servo_angle;
                } else {
                    ESP_LOGE(TAG, "servo.setAngle failed: %s", esp_err_to_name(err));
                }
            }
            last_servo_count = current_count;
        }

        bool current_btn = encoder.isButtonPressed();
        if (current_btn && !last_btn) {
            button_press_start = get_millis();
            long_press_handled = false;
        }

        if (current_btn && !long_press_handled) {
            unsigned long press_time = get_millis() - button_press_start;
            if (press_time >= BUTTON_LONG_PRESS_MS) {
                servo_step_scale = 1.0f;
                err = encoder.reset();
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "encoder.reset failed: %s", esp_err_to_name(err));
                }
                current_servo_angle = Servo::kCenterAngle;
                err = servo.setAngle(current_servo_angle);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "servo.setAngle(center) failed: %s", esp_err_to_name(err));
                }
                last_servo_count = 0;
                long_press_handled = true;
            }
        }

        if (!current_btn && last_btn) {
            if (!long_press_handled) {
                servo_step_scale *= 0.5f;
            }
        }
        last_btn = current_btn;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
