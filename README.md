# ESP32-S3 Servo Control via Rotary Encoder (ESP-IDF + PlatformIO)

Controls a servo motor with a rotary encoder. Turning the encoder moves the servo shaft, the button adjusts sensitivity or resets to center, and a buzzer sounds at angle limits.

## Hardware

| Component | Model / Notes |
|---|---|
| Board | ESP32-S3-DevKitC-1 (N16R8, 16 MB Flash, 8 MB OPI PSRAM) |
| Rotary encoder | Quadrature encoder with push button (e.g. KY-040) |
| Servo motor | Standard hobby servo, 0–180°, 50 Hz PWM signal |
| Buzzer | Passive piezo buzzer |

## Wiring

```
Encoder:
  CLK (Channel A)  →  GPIO 5
  DT  (Channel B)  →  GPIO 4
  SW  (Button)     →  GPIO 6
  VCC              →  3.3V
  GND              →  GND

Servo:
  Signal           →  GPIO 18
  VCC              →  5V (external supply recommended)
  GND              →  GND (common with ESP32)

Buzzer:
  Signal           →  GPIO 17
  GND              →  GND
```

> **Note:** Channel A/B are intentionally swapped in firmware to match the physical encoder orientation — CW rotation increments the count, CCW decrements it.

## Behavior

| Action | Result |
|---|---|
| Rotate encoder CW | Servo moves toward 180° |
| Rotate encoder CCW | Servo moves toward 0° |
| Reach angle limit (0° or 180°) | Short buzzer beep |
| Short button press | Servo step per tick halved (`scale × 0.5`) |
| Long button press (≥ 1 s) | Step scale reset to `1.0`, servo returns to 90° |

## Software requirements

- VS Code
- PlatformIO extension
- ESP-IDF toolchain (installed automatically by PlatformIO)

## Build and run

Build:

```bash
pio run
```

Flash firmware:

```bash
pio run -t upload
```

Open serial monitor (115200 baud):

```bash
pio device monitor -b 115200
```

## Configuration

Key constants in `src/main.cpp`:

| Constant | Default | Description |
|---|---|---|
| `SERVO_COUNTS_PER_DEGREE` | `2.0` | Encoder counts required per 1° of servo movement |
| `SERVO_DIRECTION` | `-1.0` | Set to `1.0` to flip rotation direction |
| `BUTTON_LONG_PRESS_MS` | `1000` | Long-press threshold in milliseconds |
| `LIMIT_BEEP_COOLDOWN_MS` | `120` | Minimum time between buzzer beeps at limits |

Servo pulse range (in `src/servo.cpp`):

| Constant | Value | Description |
|---|---|---|
| `MIN_PULSE_US` | `500 µs` | Pulse width at 0° |
| `MAX_PULSE_US` | `2500 µs` | Pulse width at 180° |
| `SERVO_FREQ` | `50 Hz` | PWM frequency |
| `SERVO_RESOLUTION` | 13-bit | Duty cycle resolution (8192 steps) |

Platform settings (`platformio.ini`): framework `espidf`, flash mode `qio`, flash size `16MB`, monitor `115200` baud.

## Project structure

```
src/
  main.cpp          # App entry point: control loop, button handling
  servo.cpp         # LEDC PWM setup and angle control
  encoder.cpp       # PCNT quadrature decoder, RPM, direction detection
  buzzer.cpp        # LEDC buzzer setup and limit beep
include/
  servo.hpp
  encoder.hpp
  buzzer.hpp
platformio.ini                    # Board and build settings
sdkconfig.esp32-s3-devkitc-1     # ESP-IDF Kconfig options
```

## Contact

Feedback: max.savin3@gmail.com
