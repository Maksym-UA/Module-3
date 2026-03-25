#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_OUT		GPIO_NUM_16
#define BUTTON_IN	GPIO_NUM_15

//configure GPIO for LED output and provide methods to turn the LED on and off.
class Led {
    public:
        Led(gpio_num_t pin) : pin_(pin) {
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << pin_),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE
            };
            gpio_config(&io_conf);
        }
        void on() const {
            gpio_set_level(pin_, 0);
        }
        void off() const {
            gpio_set_level(pin_, 1);
        }
    private:
        const gpio_num_t pin_;
};

//configure gpio for button input, implement debouncing logic to ensure stable button
// state readings, and provide a method to check if the button is currently pressed.
class Button {
    public:
        Button(gpio_num_t pin) : pin_(pin), debouncedState_(false), rawState_(false),
        lastChangeTime_(0), debounceMs_(50) {
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << pin_),
                .mode = GPIO_MODE_INPUT,
                .pull_up_en = GPIO_PULLUP_ENABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE
            };
            gpio_config(&io_conf);
        }
        bool isPressed() {
            bool current = gpio_get_level(pin_) == 0;
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (current != rawState_) {
                rawState_ = current;
                lastChangeTime_ = now;
            }
            if (now - lastChangeTime_ > debounceMs_) {
                debouncedState_ = rawState_;
            }
            return debouncedState_;
        }
    private:
        const gpio_num_t pin_;
        bool debouncedState_;
        bool rawState_;
        uint32_t lastChangeTime_;
        const uint32_t debounceMs_;
};

extern "C"
{
    void app_main() {
        Led led(LED_OUT);
        Button button(BUTTON_IN);

        bool prevPressed = false;

        while (1) {
            bool pressed = button.isPressed();
            if (pressed) {
                led.on();
                if (!prevPressed) {
                    printf("Button Pressed\n");
                }
            } else {
                led.off();
            }
            prevPressed = pressed;
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
    }
}