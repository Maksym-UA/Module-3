# ESP32-S3: Potentiometer-Controlled PWM Motor Speed

This project reads a 10k potentiometer on ESP32-S3 ADC input and uses it to control motor speed with LEDC PWM.

## What it does

- Reads analog value from `GPIO4` (`ADC1_CH3`)
- Maps ADC range (`0..4095`) to PWM duty (`0..1023`, 10-bit)
- Outputs PWM on `GPIO5` at `20 kHz`
- Prints debug info to Serial at `115200` baud:
	- raw potentiometer value
	- speed in percent
	- applied PWM duty

## Hardware pins

- Potentiometer signal -> `GPIO4`
- Motor PWM output -> `GPIO5`
- Common GND between ESP32-S3 and driver/power stage

## Wiring diagram (ASCII)

```text
Potentiometer (10k)                ESP32-S3 DevKitC-1
-------------------               --------------------
VCC  --------------------------->  3V3
GND  --------------------------->  GND
SIG (wiper) -------------------->  GPIO4 (ADC1_CH3)

ESP32-S3 DevKitC-1                Motor driver / transistor stage
--------------------             -------------------------------
GPIO5 (PWM / LEDC) ------------->  PWM / IN control pin
GND ---------------------------->  GND
```

## Safety note

- Do **not** connect a motor directly to an ESP32 GPIO pin.
- Use a proper motor driver or transistor/MOSFET stage.
- For brushed DC motors, add a flyback diode across the motor terminals.
- Ensure the motor power supply and ESP32 share a common GND.

## Build and upload (PlatformIO)

1. Install PlatformIO (VS Code extension).
2. Open this project folder in VS Code.
3. Build:

```bash
platformio run
```

4. Upload:

```bash
platformio run --target upload
```

5. Open serial monitor:

```bash
platformio device monitor -b 115200
```

## Environment

- Board: `esp32-s3-devkitc-1`
- Framework: `arduino`

## Contact

Feedback: `max.savin3@gmail.com`
