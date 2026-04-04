#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#define LED_GPIO (gpio_num_t)18
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_FREQUENCY 1000     // 1 kHz
#define LEDC_RESOLUTION LEDC_TIMER_10_BIT      // 10-bit (0-1023)



extern "C" void app_main(void) {

  // Налаштування таймера
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);

    // Налаштування каналу
    ledc_channel_config_t channel_config = {
        .gpio_num = LED_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel_config);

    while (1) {
        // Fade-in: поступово збільшуємо яскравість з 0% до 100%
        for (int brightness = 0; brightness <= 1023; brightness += 10) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, brightness);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(20));  // 20 мс затримка між кроками
        }

        vTaskDelay(pdMS_TO_TICKS(500));  // 500 мс пауза при максимальній яскравості

        // Fade-out: поступово зменшуємо яскравість з 100% до 0%
        for (int brightness = 1023; brightness >= 0; brightness -= 10) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, brightness);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 секунда пауза перед повтором
    }
}