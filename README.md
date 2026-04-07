# ESP32-S3 Potentiometer-Controlled LED + DC Motor (ESP-IDF + PlatformIO)

This project demonstrates ADC + PWM control on ESP32-S3.
A single potentiometer input is read by ADC, and its value is mapped to PWM duty for both LED and DC motor.

## What the project does

- Initializes ADC1 one-shot mode to read potentiometer on GPIO4
- Uses ADC calibration (curve fitting) to convert raw reading to millivolts
- Maps 12-bit ADC value (`0..4095`) to 10-bit PWM duty (`0..1023`)
- Drives LED brightness and DC motor speed from the same potentiometer value
- LED and motor are configured on separate LEDC channels/timers, so outputs are independent

Current settings from `src/main.cpp`:

- Potentiometer input: `GPIO4` (`ADC1_CH3`)
- LED output: `GPIO18`, `LEDC_CHANNEL_0`, `LEDC_TIMER_0`, `1000 Hz`
- Motor output: `GPIO5`, `LEDC_CHANNEL_1`, `LEDC_TIMER_1`, `20000 Hz`
- PWM resolution: `10-bit` (duty `0..1023`)

## Hardware

- Board: ESP32-S3-DevKitC-1
- Potentiometer connected to GPIO4 (ADC input)
- LED connected to GPIO18 with current-limiting resistor (220 Ω recommended)
- DC motor driver input (PWM) connected to GPIO5
- ST2N2222A NPN transitor
- 1N4007 1 kΩ freewheeling diode

Note: connect motor via a proper driver/transistor stage, not directly to ESP32 pin.

## Software requirements

- VS Code
- PlatformIO extension
- ESP-IDF toolchain (installed by PlatformIO for this environment)

## Build and run

From the project root:

```bash
pio run
```

Upload firmware:

```bash
pio run -t upload
```

Open serial monitor (115200 baud):

```bash
pio device monitor -b 115200
```

## Configuration notes

- Framework: `espidf`
- Monitor speed: `115200`
- Flash mode/size configured for this project: `qio`, `16MB`

## Project structure

- `src/main.cpp` - ADC reading + PWM control for LED and motor
- `platformio.ini` - board and build settings
- `sdkconfig.esp32-s3-devkitc-1` - ESP-IDF configuration

## Contact

Feedback: max.savin3@gmail.com
