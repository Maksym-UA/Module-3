#pragma once

#include <driver/pulse_cnt.h>
#include <esp_err.h>


// QuadratureEncoder class manages a quadrature encoder connected to the
// ESP32's pulse counter (PCNT) peripheral.
class QuadratureEncoder {
    public:
        QuadratureEncoder() = default;

        esp_err_t setup();
        void detectDirection();
        void calculateRpm();

        esp_err_t getCount(int *count) const;
        int getCount() const; // compatibility helper

        bool isButtonPressed() const;
        esp_err_t reset();

        static bool onFullRotation(pcnt_unit_handle_t,
                                const pcnt_watch_event_data_t *edata,
                                void *);

    private:
        pcnt_unit_handle_t unit_ = nullptr;
        pcnt_channel_handle_t chan_a_ = nullptr;
        pcnt_channel_handle_t chan_b_ = nullptr;

        int last_position_ = 0;
        int last_count_ = 0;
        int last_direction_state_ = 0;
        unsigned long last_direction_time_ = 0;
        unsigned long last_rpm_time_ = 0;
};
