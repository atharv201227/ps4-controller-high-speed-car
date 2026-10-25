# ESP32 4WD Obstacle-Racing Car

A Bluetooth-controlled 4WD obstacle-racing car built around an **ESP32 DevKit V1**, an **EvoFox Play 2 wireless controller**, four DC geared motors, and two L298N motor drivers. The project uses **Bluepad32** to receive controller input.

## Features

- Bluetooth game-controller input through Bluepad32
- Differential steering for forward, reverse, and turning
- PWM motor control (0–255)
- Three speed modes
- Gradual acceleration ramp
- Independent left/right motor calibration values
- Stops motor commands when the controller disconnects
- Emergency stop while the configured button is held

> **Important:** This is the README for the sketch as currently written. Hardware and controller compatibility must be verified with a safe bench test before driving.

## Hardware

- ESP32 DevKit V1
- 4 × DC geared motors (the current build uses 12 V Johnson-style motors)
- 2 × L298N dual H-bridge motor-driver modules
- EvoFox Play 2 wireless controller
- Two suitable LiPo batteries, if retaining the existing one-battery-per-driver arrangement
- Suitable regulated supply for the ESP32
- Chassis, wheels, wiring, and connectors

### Power and safety

- Never connect the positive terminals of the two motor batteries together when each battery is powering a separate driver.
- Connect ESP32 ground and both L298N grounds together so the control signals have a common reference.
- Power the ESP32 through a suitable regulated supply. Do not connect a battery directly to the ESP32's 3.3 V pin.
- A 3-cell (3S) LiPo is about 11.1 V nominal and 12.6 V fully charged.
- **Do not use a swollen or damaged LiPo battery.** Charge and handle LiPo batteries using a suitable balance charger and safe charging practices.
- L298N modules have significant voltage drop and may be unsuitable for motors with high startup or stall current. Confirm each motor's current requirements and the driver's limits before running under load. Do not intentionally stall a motor to measure current.
- Disconnect the batteries before changing motor wiring.
- Start tests with the chassis securely raised so the wheels cannot touch the ground. Keep hands, cables, and loose objects away from wheels and shafts.
- Stop testing if wiring, batteries, motors, or drivers become excessively hot, or if the ESP32 resets.

## Software requirements

- Arduino IDE
- ESP32 Arduino board support compatible with the installed Bluepad32 package
- Bluepad32 library/package

Bluepad32 Arduino documentation: <https://bluepad32.readthedocs.io/en/latest/plat_arduino/>

Install and select the ESP32 + Bluepad32 board package as described in the Bluepad32 documentation. The sketch contains conditional PWM code for ESP32 Arduino core 2.x and 3.x, but package/API differences can still require adjustments.

## GPIO wiring

The sketch assumes the following pin assignments.

| Function | ESP32 GPIO | L298N terminal |
|---|---:|---|
| Front-left motor PWM | 25 | Left driver ENA |
| Front-left direction 1 | 26 | Left driver IN1 |
| Front-left direction 2 | 27 | Left driver IN2 |
| Rear-left motor PWM | 14 | Left driver ENB |
| Rear-left direction 1 | 16 | Left driver IN3 |
| Rear-left direction 2 | 17 | Left driver IN4 |
| Front-right motor PWM | 32 | Right driver ENA |
| Front-right direction 1 | 33 | Right driver IN1 |
| Front-right direction 2 | 18 | Right driver IN2 |
| Rear-right motor PWM | 19 | Right driver ENB |
| Rear-right direction 1 | 21 | Right driver IN3 |
| Rear-right direction 2 | 22 | Right driver IN4 |

Connect each motor to its corresponding output pair on the appropriate L298N:

- Left driver OUT1/OUT2: front-left motor
- Left driver OUT3/OUT4: rear-left motor
- Right driver OUT1/OUT2: front-right motor
- Right driver OUT3/OUT4: rear-right motor

Remove the **ENA and ENB jumpers** on both L298N boards so the ESP32 can control the enable pins with PWM. Check your particular L298N board's 5 V regulator/jumper arrangement against its documentation; do not connect supplies together blindly.

The two motor-driver battery positive supplies remain separate in the current setup. The grounds must be common with the ESP32.

## Controller mapping

| Controller input | Intended behavior |
|---|---|
| Left stick up | Forward |
| Left stick down | Reverse |
| Left stick left | Turn left |
| Left stick right | Turn right |
| Left stick diagonally | Drive and steer at the same time |
| L1 | Slow speed mode |
| Neither L1 nor R1 | Normal speed mode |
| R1 | Fast speed mode |
| `ctl->b()` | Emergency stop while held |

Button labels and axis directions can differ by controller mapping. Verify them in a safe test. The code assumes `ctl->b()` corresponds to the desired emergency-stop button on the EvoFox controller; change the mapping if Bluepad32 reports a different button.

## Adjustable settings in the sketch

| Setting | Purpose | Current value |
|---|---|---:|
| `MAX_PWM` | Maximum PWM output | 255 |
| `SLOW_SPEED` | Slow mode limit | 45% |
| `NORMAL_SPEED` | Normal mode limit | 75% |
| `FAST_SPEED` | Fast mode limit | 100% |
| `JOYSTICK_DEAD_ZONE` | Ignores small joystick movements | 60 |
| `RAMP_STEP` | PWM change per ramp update | 5 |
| `RAMP_INTERVAL_MS` | Time between ramp updates | 10 ms |
| `leftCalibration` | Left-side calibration percentage | 100 |
| `rightCalibration` | Right-side calibration percentage | 100 |

Increase `RAMP_STEP` or reduce `RAMP_INTERVAL_MS` for quicker response; reduce `RAMP_STEP` or increase the interval for gentler acceleration. Change calibration values in small increments if one side consistently runs faster than the other.

The current ramp controls the left and right sides as groups. It does not provide separate closed-loop speed regulation for each motor, and there are no encoders in this design.

## Upload and first test

1. Check all wiring against the GPIO table and your actual driver board.
2. Ensure the ENA/ENB jumpers are removed and the grounds are common.
3. Keep the car securely raised and clear of obstacles.
4. Select the correct ESP32 + Bluepad32 board in Arduino IDE and upload the sketch.
5. Open Serial Monitor at **115200 baud**.
6. Pair/connect the EvoFox controller according to the Bluepad32 documentation and observe the connection messages.
7. Test forward and reverse at low risk, then left and right.
8. Verify the emergency-stop button and disconnect failsafe.
9. If a motor rotates in the wrong direction, disconnect all batteries before swapping that motor's wires.
10. Only after these checks, conduct a brief low-speed test in a clear area.

## Troubleshooting

### Controller does not connect
- Confirm the correct Bluepad32 board package is installed and selected.
- Check the Serial Monitor for connection messages.
- Confirm the controller model is supported by the installed Bluepad32 version.
- If pairing fails, follow the Bluepad32 documentation. The sketch calls `BP32.forgetBluetoothKeys()` at startup, which clears saved Bluetooth keys; remove that call if you need to preserve existing bonds.

### Car turns the wrong way
- Check motor polarity and confirm the left and right motors are assigned correctly.
- Test with the wheels raised and change wiring only with battery power disconnected.
- Confirm the controller's X-axis sign is correct for your controller mapping.

### Car is slow under load
- Full PWM does not guarantee full battery voltage at the motors. L298N voltage loss, motor torque/current requirements, battery condition, wiring, wheel traction, and chassis drag can all affect speed.
- Check for binding wheels and loose connections.
- Do not assume that a cool L298N is necessarily capable of supplying the required current.
- If upgrading drivers, choose them based on each motor's measured/specification startup and stall current, not just its no-load current or nominal voltage.

### ESP32 resets or motors behave erratically
- Check the ESP32's regulated supply, ground connections, wiring, and electrical noise.
- Keep motor power wiring appropriately sized and connections secure.
- Stop testing if a battery, driver, wire, or motor becomes excessively hot.

## Current limitations

- No wheel encoders or closed-loop speed control
- No distance sensors or autonomous obstacle avoidance
- No battery-voltage monitoring
- No configurable on-screen settings
- Pivot-turn behavior depends on the joystick mixing and motor response; it is not a separate selectable mode
- Actual controller button mappings must be verified on the target controller
- L298N performance may limit speed and torque under load

## License

Add a license before publishing or redistributing the project. Until a license is added, no explicit reuse license is granted.
