# ESP32 Line Follower with Bluepad32 Controller

A five-sensor line-following robot built using a classic ESP32, a TB6612FNG motor driver, and Bluetooth gamepad support through Bluepad32.

**Controller:** EvoFox Play 2 Wireless Gaming Controller
**Development environment:** Arduino IDE 2.2.10
**Microcontroller:** Classic ESP32
**Motor driver:** TB6612FNG
**Line sensors:** Five digital IR sensors

## Features

* Five-sensor line following.
* PID-based steering control.
* Bluetooth controller connectivity using Bluepad32.
* Start/stop control using the controller's Start button.
* Automatic motor stop when the controller disconnects.
* No web server required.

> The Start-button mapping and disconnect-stop behavior must be tested on the actual hardware before running the robot at speed.

## 1. Hardware Required

| Component                       | Quantity | Purpose                             |
| ------------------------------- | -------: | ----------------------------------- |
| Classic ESP32 development board |        1 | Main controller                     |
| Digital IR line sensors         |        5 | Line detection                      |
| TB6612FNG motor driver          |        1 | Motor control                       |
| DC geared motors                |        2 | Robot movement                      |
| EvoFox Play 2 controller        |        1 | Wireless control                    |
| Suitable battery                |        1 | Power supply                        |
| Buck converter                  |        1 | Regulated motor supply, if required |
| Robot chassis and wheels        |    1 set | Mechanical assembly                 |

## 2. Wiring

### TB6612FNG Motor Driver

| Driver Pin  | ESP32 Connection       |
| ----------- | ---------------------- |
| PWMA        | GPIO 25                |
| AIN1        | GPIO 26                |
| AIN2        | GPIO 27                |
| PWMB        | GPIO 14                |
| BIN1        | GPIO 32                |
| BIN2        | GPIO 33                |
| STBY        | GPIO 13                |
| VCC         | 3.3 V                  |
| VM          | Regulated motor supply |
| GND         | Common GND             |
| AO1 and AO2 | Left motor             |
| BO1 and BO2 | Right motor            |

Use a regulated motor supply appropriate for your motors. For 6 V motors, use approximately 6 V at VM, subject to the motor and driver specifications.

### Five IR Sensors

| Sensor | Position  | ESP32 GPIO |
| ------ | --------- | ---------: |
| S1     | Far left  |         34 |
| S2     | Left      |         35 |
| S3     | Centre    |         36 |
| S4     | Right     |         39 |
| S5     | Far right |          4 |

**Important:**

* ESP32 GPIO pins are not 5 V tolerant.
* Ensure the sensor output signals do not exceed 3.3 V.
* GPIO 34, 35, 36 and 39 are input-only pins.
* Connect the grounds of the ESP32, sensors, and motor driver together.

## 3. Software Requirements

Install or prepare:

1. Arduino IDE 2.2.10.
2. A compatible ESP32 + Bluepad32 board package.
3. A USB data cable.
4. The Bluepad32 controller example for testing.

Official documentation:

* [Bluepad32 Arduino Platform Guide](https://github.com/ricardoquesada/bluepad32/blob/main/docs/plat_arduino.md)
* [Bluepad32 Getting Started](https://bluepad32.readthedocs.io/en/latest/getting_started/)
* [Bluepad32 Arduino Programmer's Guide](https://bluepad32.readthedocs.io/en/latest/programmers_guide_arduino/)

### Installation

1. Open Arduino IDE.
2. Go to **File → Preferences**.
3. Locate **Additional boards manager URLs**.
4. Add the Bluepad32 board-package URL specified in the official Arduino platform guide.
5. Open **Tools → Board → Boards Manager**.
6. Search for and install the **ESP32 + Bluepad32** board package according to the official guide.
7. Select the board entry matching your classic ESP32.
8. Connect the ESP32 and select the correct COM port.
9. Open the Bluepad32 controller test example.
10. Compile and upload the example.
11. Open **Tools → Serial Monitor** and select **115200 baud**.

**Note:** Bluepad32 requires a compatible board package. Installing a similarly named library alone may not be sufficient.

## 4. Pairing the EvoFox Controller

1. Upload the Bluepad32 controller test example.
2. Open Serial Monitor at 115200 baud.
3. Turn on the EvoFox Play 2 controller in its Bluetooth pairing mode.
4. Wait for the ESP32 to report a controller connection.
5. Press the controller's buttons and move its sticks.
6. Observe the reported input values.
7. Identify which input corresponds to the Start button.

Confirm the actual button mapping before integrating the controller into the robot firmware.

## 5. Intended Robot Operation

The integrated firmware is intended to work as follows:

1. When powered on, the robot remains stopped.
2. Pressing the controller's Start button starts the line-following routine.
3. Pressing Start again stops the robot.
4. The five IR sensors detect the line.
5. The PID controller adjusts the left and right motor speeds.
6. If the controller disconnects, the motors stop.
7. The robot remains stopped until it is started again.

The final firmware must implement and verify these behaviors before normal operation.

## 6. Power Supply and Safety

* Never connect an 11.1 V battery directly to 6 V motors.
* Use an appropriately rated buck converter to supply the required motor voltage.
* Check the motor driver's current rating against the motors' operating and stall currents.
* Use a suitable regulator for the ESP32 if powering it from the robot battery.
* Keep all circuit grounds connected together.
* Test the robot with its wheels raised before placing it on the ground.
* Verify that the controller disconnect causes the motors to stop.
* Use a physical power switch so the robot can be switched off quickly.

A software stop is not a substitute for a physical emergency stop.

## 7. Troubleshooting

### Error: `Bluepad32.h: No such file or directory`

Check that the correct Bluepad32-compatible board package is installed and selected. Follow the official installation guide.

### Bluepad32 board package is missing

Check the package URL in Arduino IDE Preferences and review the Boards Manager installation messages.

### Controller does not connect

* Confirm the controller is in Bluetooth pairing mode.
* Keep the controller close to the ESP32.
* Check Serial Monitor at 115200 baud.
* Test with the Bluepad32 example before testing the complete robot.

### Motors do not move

* Check the battery and regulated motor supply.
* Verify the common ground.
* Check the STBY pin and motor connections.
* Confirm the selected GPIO numbers match the wiring.

### Robot spins instead of following the line

* Check the sensor output states.
* Confirm the correct active sensor polarity.
* Verify the left and right motor directions.
* Test each sensor independently.
* Tune PID values only after the sensors and motor directions work correctly.

### Robot loses the line at sharp turns

Check whether the outer sensors detect the line at the turn. The line-loss recovery logic and PID settings may need adjustment.

## 8. Recommended Testing Order

1. Inspect all wiring with power disconnected.
2. Test the motors at low speed with the wheels raised.
3. Verify all five IR sensors.
4. Upload the Bluepad32 controller test example.
5. Confirm controller pairing and identify the Start button input.
6. Integrate Bluetooth control with the line-following firmware.
7. Test start, stop, and disconnect behavior with the wheels raised.
8. Test line following at low speed.
9. Increase speed gradually after the robot tracks reliably.

## License

No project license has been specified yet.
