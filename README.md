# ESP32-S3 Light-Control Project (ESP-IDF + PlatformIO)

This project reads an LDR sensor on ESP32-S3, filters the ADC signal with a
Simple Moving Average (SMA), and controls an LED with hysteresis to avoid
flicker.

## What the project does

- Reads light level from ADC (GPIO4 / ADC1 CH3)
- Converts raw ADC value to millivolts using calibration (curve fitting)
- Applies SMA filter with a 10-sample window
- Turns LED ON in dark conditions and OFF in bright conditions
- Logs raw and filtered voltage to serial output

Current control values from `src/main.cpp`:

- `LED_GPIO = 18`
- `THRESHOLD_LOW = 1400 mV` (turn LED ON)
- `THRESHOLD_HIGH = 1800 mV` (turn LED OFF)
- Sampling interval: `100 ms`

## Hardware

- Board: ESP32-S3-DevKitC-1
- LDR voltage divider connected to GPIO4 (ADC input)
- LED connected to GPIO18 (with resistor)

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

Expected log format:

```text
I (....) ADC: Voltage: 1560 mV | Filtered: 1512 mV
```

## Configuration notes

- Framework: `espidf`
- Monitor speed: `115200`
- Flash mode/size configured for this project: `qio`, `16MB`

Main configuration file:

- `platformio.ini`

## Project structure

- `src/main.cpp` - application logic (ADC read, filtering, LED control)
- `platformio.ini` - board and build settings
- `sdkconfig.esp32-s3-devkitc-1` - ESP-IDF configuration

## Contact

Feedback: max.savin3@gmail.com
