# ESP32-S3 Jingle Bells with PWM (LEDC) (ESP-IDF + PlatformIO)

This project plays the "Jingle Bells" melody on a buzzer using the ESP32-S3 LEDC PWM peripheral.
It runs in `app_main()` and uses `usleep()` for timing (no explicit FreeRTOS task or `vTaskDelay()`).

## Hardware

- Board: ESP32-S3-DevKitC-1
- Buzzer on GPIO4

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
- The fundamental issue is that `ledc_set_freq()` can fail to retune frequency on an already-configured timer for these note values.
- Current approach: reconfigure the LEDC timer (`ledc_timer_config`) per note, then set duty/update duty to produce sound.

## Project structure

- `src/main.cpp` - LEDC PWM buzzer initialization and Jingle Bells melody playback
- `platformio.ini` - board and build settings
- `sdkconfig.esp32-s3-devkitc-1` - ESP-IDF configuration

## Contact

Feedback: max.savin3@gmail.com
