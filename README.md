# ESP32-S3 Generate melody with DAC (ESP-IDF + PlatformIO)

This project plays "Twinkle, Twinkle, Little Star" melody via DAC on ESP32-S3.




## Hardware

- Board: ESP32-S3-DevKitC-1
- Buzzer


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
- The fundamental issue is that ledc_set_freq() can't dynamically change the frequency on an already-configured LEDC timer with the current settings. The 100 Hz base frequency doesn't provide enough clock divider resolution to reach these specific audio frequencies.
- Changed the approach: instead of trying to dynamically change the frequency with ledc_set_freq(), the code now reconfigures the entire timer for each note. This allows the LEDC hardware to automatically calculate the proper clock dividers for each specific frequency. Build and test—this should make all the notes play.

## Project structure

- `src/main.cpp` - ADC reading + PWM control for LED and motor
- `platformio.ini` - board and build settings
- `sdkconfig.esp32-s3-devkitc-1` - ESP-IDF configuration

## Contact

Feedback: max.savin3@gmail.com
