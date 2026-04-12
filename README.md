# ESP32-S3 Servo Control with Potentiometer (LEDC PWM + ADC) (ESP-IDF + PlatformIO)

This project controls a servo motor on ESP32-S3 using a potentiometer via the ADC unit and LEDC PWM peripheral.
The firmware reads analog input from a potentiometer on GPIO4 and converts it to servo angle (0-180°) with 1:1 mapping.

## Hardware

- Board: ESP32-S3-DevKitC-1
- Servo motor (signal on GPIO18)
- Potentiometer (analog input on GPIO4 / ADC1_CH3)

## Wiring

```
Potentiometer:
  - One end → 3.3V
  - Wiper (middle pin) → GPIO4
  - Other end → GND

Servo:
  - Signal wire → GPIO18
  - VCC → 5V (separate power recommended)
  - GND → GND (common ground with ESP32-S3)
```

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

## Configuration and Calibration

### Initial Setup (Calibration Required)

The code needs to be calibrated to map the potentiometer's actual ADC range to 0-180° servo angle.

1. **Rotate potentiometer fully LEFT** and note the raw ADC value from serial log (e.g., `Pot: raw=520`)
2. **Rotate potentiometer fully RIGHT** and note the raw ADC value (e.g., `Pot: raw=3580`)
3. **Update constants** in `src/main.cpp` (lines 31-32):
   ```cpp
   static int adc_min_value = 520;    // Left extreme
   static int adc_max_value = 3580;   // Right extreme
   ```
4. **Rebuild and upload** with corrected values
5. **Test**: potentiometer should now sweep servo from 0° to 180° with 1:1 correspondence

### Configuration notes

- Framework: `espidf`
- Monitor speed: `115200`
- Flash mode/size: `qio`, `16MB`
- **PWM frequency**: `50 Hz` (servo standard)
- **ADC channel**: GPIO4 (ADC1_CH3), 12-bit resolution (0-4095)
- **LEDC resolution**: 13-bit (8192 duty steps)
- **Pulse mapping**: 1000 µs (0°) to 2000 µs (180°), period 20000 µs
- **Polling rate**: 100 ms between pot reads and servo updates
- **ADC attenuation**: DB_12 (full 0–3.3 V range)

## Project structure

- `src/main.cpp` - LEDC timer/channel setup and servo angle sweep logic
- `platformio.ini` - board and build settings
- `sdkconfig.esp32-s3-devkitc-1` - ESP-IDF configuration

## Contact

Feedback: max.savin3@gmail.com
