#include <unistd.h>
#include "driver/ledc.h"
#include "esp_log.h"

#define BUZZER_GPIO 4
#define BUZZER_LEDC_MODE LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_TIMER LEDC_TIMER_0
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_DUTY_RES LEDC_TIMER_8_BIT
#define BUZZER_DUTY_ON 128

static const char *TAG = "pwm_buzzer";

// Музичні ноти (частоти в Hz)
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_C5 523

static void playNote(int frequency, int durationMs)
{
    if (frequency <= 0) {
        ledc_stop(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0);
        usleep(durationMs * 1000);
        return;
    }
    // The fundamental issue is that ledc_set_freq() can't dynamically change the frequency
    // on an already-configured LEDC timer with the current settings. The 100 Hz base frequency
    // doesn't provide enough clock divider resolution to reach these specific audio frequencies.

    // Reconfigure timer with the desired frequency
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = BUZZER_LEDC_MODE;
    ledc_timer.timer_num = BUZZER_LEDC_TIMER;
    ledc_timer.duty_resolution = BUZZER_DUTY_RES;
    ledc_timer.freq_hz = frequency;
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ESP_LOGI(TAG, "Playing frequency: %d Hz", frequency);

    ESP_ERROR_CHECK(ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, BUZZER_DUTY_ON));
    ESP_ERROR_CHECK(ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL));

    usleep(durationMs * 1000);

    ESP_ERROR_CHECK(ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0));
    ESP_ERROR_CHECK(ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL));
    usleep(50 * 1000);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Initializing PWM buzzer...");

    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = BUZZER_LEDC_MODE;
    ledc_timer.timer_num = BUZZER_LEDC_TIMER;
    ledc_timer.duty_resolution = BUZZER_DUTY_RES;
    ledc_timer.freq_hz = 100;  // Low base frequency for better divider resolution
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num = BUZZER_GPIO;
    ledc_channel.speed_mode = BUZZER_LEDC_MODE;
    ledc_channel.channel = BUZZER_LEDC_CHANNEL;
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.timer_sel = BUZZER_LEDC_TIMER;
    ledc_channel.duty = 0;
    ledc_channel.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_LOGI(TAG, "PWM buzzer ready on GPIO%d", BUZZER_GPIO);

    while (1) {
        // Мелодія "Jingle Bells" ( ближчий до оригіналу)
        // Jingle bells, jingle bells, jingle all the way
        // E E E | E E E | E G C D E
        playNote(NOTE_E4, 250);
        playNote(NOTE_E4, 250);
        playNote(NOTE_E4, 500);

        playNote(NOTE_E4, 250);
        playNote(NOTE_E4, 250);
        playNote(NOTE_E4, 500);

        playNote(NOTE_E4, 250);
        playNote(NOTE_G4, 250);
        playNote(NOTE_C4, 250);
        playNote(NOTE_D4, 250);
        playNote(NOTE_E4, 750);

        // Oh what fun it is to ride in a one-horse open sleigh
        // F F F F F E E | E G G F D C
        playNote(NOTE_F4, 250);
        playNote(NOTE_F4, 250);
        playNote(NOTE_F4, 250);
        playNote(NOTE_F4, 250);
        playNote(NOTE_F4, 250);
        playNote(NOTE_E4, 250);
        playNote(NOTE_E4, 250);

        playNote(NOTE_E4, 250);
        playNote(NOTE_G4, 250);
        playNote(NOTE_G4, 250);
        playNote(NOTE_F4, 250);
        playNote(NOTE_D4, 250);
        playNote(NOTE_C4, 1000);

        usleep(2000 * 1000);
    }
}

