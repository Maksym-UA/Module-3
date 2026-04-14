#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <driver/pulse_cnt.h>
#include <stdio.h>
#include <driver/gpio.h>



#define ENCODER_GPIO_A 9
#define ENCODER_GPIO_B 10
#define PCNT_UNIT PCNT_UNIT_0

pcnt_unit_handle_t pcnt_unit = NULL;
pcnt_channel_handle_t pcnt_channel_0 = NULL;
pcnt_channel_handle_t pcnt_channel_1 = NULL;

void encoder_monitor_task(void *arg) {
    while (1) {
        int count;
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &count));
        ESP_LOGI("ENCODER", "Position: %d steps", count);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setup_encoder_pcnt() {
    pcnt_unit_config_t unit_config = {
        .high_limit = 10000,
        .low_limit = -10000,
        .accum_count = true,
        .flags = {
            .accum_count = 0,
        }
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_chan_config_t chan_config_0 = {
        .edge_gpio_num = ENCODER_GPIO_A,
        .level_gpio_num = ENCODER_GPIO_B,
        .flags = {
            .invert_edge_input = 0,
            .invert_level_input = 0,
            .virt_edge_io_level = 0,
            .virt_level_io_level = 0,
            .io_loop_back = 0,
        },
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config_0, &pcnt_channel_0));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel_0,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE
    ));

    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel_0,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));

    pcnt_chan_config_t chan_config_1 = {
        .edge_gpio_num = ENCODER_GPIO_B,
        .level_gpio_num = ENCODER_GPIO_A,
        .flags = {
            .invert_edge_input = 0,
            .invert_level_input = 0,
            .virt_edge_io_level = 0,
            .virt_level_io_level = 0,
            .io_loop_back = 0,
        },
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config_1, &pcnt_channel_1));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel_1,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE
    ));

    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel_1,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
    ESP_LOGI("ENCODER", "PCNT X4 Quadrature Mode Initialized");

}