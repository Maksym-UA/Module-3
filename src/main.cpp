#include <stdio.h>
#include <atomic>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define BUTTON_GPIO 16
#define DEBOUNCE_DELAY_MS 50

class Button {
public:
  Button(int gpio_num, uint32_t debounce_ms = DEBOUNCE_DELAY_MS)
    : gpio_num_(gpio_num), debounce_ms_(debounce_ms), press_count_(0), event_flag_(false) {
    // Configure button GPIO
    gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << gpio_num_),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);

    // Create debounce timer
    esp_timer_create_args_t debounce_timer_args = {};
    debounce_timer_args.callback = &Button::debounce_timer_callback_static;
    debounce_timer_args.arg = this;
    debounce_timer_args.dispatch_method = ESP_TIMER_TASK;
    debounce_timer_args.name = "debounce_timer";
    debounce_timer_args.skip_unhandled_events = false;
    esp_timer_create(&debounce_timer_args, &debounce_timer_);

    // Install GPIO ISR service and add handler (only once globally)
    static bool isr_service_installed = false;
    if (!isr_service_installed) {
      gpio_install_isr_service(0);
      isr_service_installed = true;
    }
    gpio_isr_handler_add((gpio_num_t)gpio_num_, Button::gpio_isr_handler_static, this);
  }

  uint32_t get_press_count() const { return press_count_; }
  bool get_and_clear_event() {
    bool was_set = event_flag_;
    event_flag_ = false;
    return was_set;
  }

private:
  int gpio_num_;
  uint32_t debounce_ms_;
  esp_timer_handle_t debounce_timer_;
  std::atomic<uint32_t> press_count_;
  std::atomic<bool> event_flag_;

  static void IRAM_ATTR gpio_isr_handler_static(void* arg) {
    Button* self = static_cast<Button*>(arg);
    esp_timer_stop(self->debounce_timer_);
    esp_timer_start_once(self->debounce_timer_, self->debounce_ms_ * 1000);
  }

  static void debounce_timer_callback_static(void* arg) {
    Button* self = static_cast<Button*>(arg);
    // Check if button is still pressed (active low)
    if (gpio_get_level((gpio_num_t)self->gpio_num_) == 0) {
      self->press_count_++;
      self->event_flag_ = true;
    }
  }
};

extern "C" void app_main() {
  Button button(BUTTON_GPIO);
  while (1) {
    if (button.get_and_clear_event()) {
      printf("Button pressed! Total presses: %lu\n", button.get_press_count());
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}