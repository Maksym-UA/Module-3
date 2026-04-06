#include <Arduino.h>
#include "driver/ledc.h"

#define MOTOR_PWM_PIN 5
#define POT_PIN 4
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER 0
#define LEDC_FREQUENCY 20000    // 20 kHz для мотора
#define LEDC_RESOLUTION 10      // 10-bit

void setup() {
    Serial.begin(115200);

    // Налаштування таймера для мотора
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = (ledc_timer_bit_t)LEDC_RESOLUTION,
        .timer_num = (ledc_timer_t)LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,      // 20 kHz для мотора
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);

    // Налаштування каналу для мотора
    ledc_channel_config_t channel_config = {
        .gpio_num = MOTOR_PWM_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = (ledc_timer_t)LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel_config);

    pinMode(POT_PIN, INPUT);
}

void loop() {
    // Читаємо потенціометр
    int potValue = analogRead(POT_PIN);  // 0-4095

    // Маштабуємо до duty cycle (0-1023)
    int dutyCycle = map(potValue, 0, 4095, 0, 1023);

    // Встановлюємо швидкість мотора
    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)LEDC_CHANNEL, dutyCycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)LEDC_CHANNEL);

    // Виводимо значення
    Serial.printf("Pot: %4d → Speed: %3d%% → Duty: %4d\n",
                  potValue,
                  (dutyCycle * 100) / 1023,
                  dutyCycle);

    delay(100);
}