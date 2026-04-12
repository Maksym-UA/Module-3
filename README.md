# ESP32-S3 Servo Control with LEDC PWM (ESP-IDF + PlatformIO)

This project controls a servo motor on ESP32-S3 using the LEDC PWM peripheral.
The firmware initializes a 50 Hz PWM signal and continuously sweeps the servo angle from 0° to 180° and back.

## Hardware

- Board: ESP32-S3-DevKitC-1
- Servo motor (signal on GPIO18)

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
- PWM frequency: `50 Hz` (servo standard)
- LEDC resolution: `13-bit` (`8192` duty steps)
- Pulse mapping: `1000 us` (0°) to `2000 us` (180°), period `20000 us`
- Sweep step: `10°` every `200 ms`

## Project structure

- `src/main.cpp` - LEDC timer/channel setup and servo angle sweep logic
- `platformio.ini` - board and build settings
- `sdkconfig.esp32-s3-devkitc-1` - ESP-IDF configuration

## Contact

Feedback: max.savin3@gmail.com
