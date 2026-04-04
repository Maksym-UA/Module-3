# ESP32-S3 LED PWM Fade Project (ESP-IDF + PlatformIO)

This project demonstrates PWM-based LED control using the LEDC peripheral.
The LED fades smoothly in and out with configurable timing.

## What the project does

- Initializes LEDC (PWM) timer on ESP32-S3
- Fades LED brightness from 0% to 100% (fade-in)
- Holds at max brightness for 500 ms
- Fades LED brightness from 100% to 0% (fade-out)
- Waits 1 second, then repeats

Current PWM settings from `src/main.cpp`:

- `LED_GPIO = 18` (output pin)
- `LEDC_FREQUENCY = 1000 Hz` (1 kHz)
- `LEDC_RESOLUTION = 10-bit` (0–1023 duty cycle)
- `STEP_DELAY = 20 ms` (smooth fade transitions)

## Hardware

- Board: ESP32-S3-DevKitC-1
- LED connected to GPIO18 with current-limiting resistor (220 Ω recommended)

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

- `src/main.cpp` - LED PWM fade logic using LEDC driver
- `platformio.ini` - board and build settings
- `sdkconfig.esp32-s3-devkitc-1` - ESP-IDF configuration

## Contact

Feedback: max.savin3@gmail.com
