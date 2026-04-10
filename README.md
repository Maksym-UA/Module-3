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

## Project structure

- `src/main.cpp` - ADC reading + PWM control for LED and motor
- `platformio.ini` - board and build settings
- `sdkconfig.esp32-s3-devkitc-1` - ESP-IDF configuration

## Contact

Feedback: max.savin3@gmail.com
