#pragma once

#include <driver/pulse_cnt.h>

class QuadratureEncoder {
public:
    QuadratureEncoder();

    void detectDirection();
    void calculateRpm();
    int getCount() const;
    bool isButtonPressed() const;
    void reset();

private:
    pcnt_unit_handle_t unit_ = nullptr;
    pcnt_channel_handle_t chan_a_ = nullptr;
    pcnt_channel_handle_t chan_b_ = nullptr;

    int last_count_ = 0;
    int last_position_ = 0;
    int last_direction_state_ = 0;
    unsigned long last_direction_time_ = 0;
    unsigned long last_rpm_time_ = 0;

    static bool onFullRotation(pcnt_unit_handle_t unit,
                               const pcnt_watch_event_data_t *edata,
                               void *user_ctx);
};
