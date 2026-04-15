#pragma once

#include <cstdint>

class Servo {
public:
    static constexpr float kMinAngle = 0.0f;
    static constexpr float kMaxAngle = 180.0f;
    static constexpr float kCenterAngle = 90.0f;

    void setup();
    void setAngle(float angle);
    float clampAngle(float angle) const;

private:
    static uint32_t angleToDuty(float angle);
};
