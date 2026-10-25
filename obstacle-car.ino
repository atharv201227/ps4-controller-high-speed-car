/*
  ESP32 4WD OBSTACLE-RACING CAR
  Controller: EvoFox Play 2 (via Bluepad32)
  Board: ESP32 DevKit V1
  Motor drivers: 2 x L298N
  Motors: 4 DC geared motors

  FEATURES
  - Bluetooth controller support
  - Differential steering
  - Adjustable acceleration
  - Three speed modes
  - Pivot turning when steering with zero throttle
  - Independent left/right motor calibration
  - Controller-disconnect failsafe
  - Emergency stop using B/Circle button

  IMPORTANT:
  Test with wheels raised before ground operation.
*/

#include <Bluepad32.h>
#include <esp_arduino_version.h>

// =====================================================
// CONTROLLER
// =====================================================

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

bool controllerConnected = false;

// =====================================================
// MOTOR DRIVER GPIO ASSIGNMENTS
// =====================================================

// LEFT L298N

// Front-left motor
const int LF_EN  = 25;
const int LF_IN1 = 26;
const int LF_IN2 = 27;

// Rear-left motor
const int LR_EN  = 14;
const int LR_IN1 = 16;
const int LR_IN2 = 17;


// RIGHT L298N

// Front-right motor
const int RF_EN  = 32;
const int RF_IN1 = 33;
const int RF_IN2 = 18;

// Rear-right motor
const int RR_EN  = 19;
const int RR_IN1 = 21;
const int RR_IN2 = 22;


// =====================================================
// PWM CONFIGURATION
// =====================================================

const int PWM_FREQUENCY = 1000;
const int PWM_RESOLUTION = 8;

// PWM channels for ESP32 Arduino core 2.x
const int CH_LF = 0;
const int CH_LR = 1;
const int CH_RF = 2;
const int CH_RR = 3;

const int MAX_PWM = 255;


// =====================================================
// CONTROLLER SETTINGS
// =====================================================

const int JOYSTICK_DEAD_ZONE = 60;

// Speed modes, percentage of maximum PWM.
const int SLOW_SPEED   = 45;
const int NORMAL_SPEED = 75;
const int FAST_SPEED   = 100;

int speedLimit = NORMAL_SPEED;


// =====================================================
// ACCELERATION SETTINGS
// =====================================================

// Increase for faster acceleration.
// Decrease for smoother acceleration.
const int RAMP_STEP = 5;

const unsigned long RAMP_INTERVAL_MS = 10;


// =====================================================
// MOTOR CALIBRATION
// =====================================================

// Adjust these if one side is faster than the other.
// Valid initial range: 80 to 120.

int leftCalibration  = 100;
int rightCalibration = 100;


// =====================================================
// MOTOR STATE
// =====================================================

int currentLeftPWM  = 0;
int currentRightPWM = 0;

unsigned long lastRampTime = 0;

bool emergencyStop = false;


// =====================================================
// PWM INITIALIZATION
// Supports ESP32 Arduino core 2.x and 3.x
// =====================================================

void setupPWM() {

#if ESP_ARDUINO_VERSION_MAJOR >= 3

  bool ok = true;

  ok &= ledcAttach(LF_EN, PWM_FREQUENCY, PWM_RESOLUTION);
  ok &= ledcAttach(LR_EN, PWM_FREQUENCY, PWM_RESOLUTION);
  ok &= ledcAttach(RF_EN, PWM_FREQUENCY, PWM_RESOLUTION);
  ok &= ledcAttach(RR_EN, PWM_FREQUENCY, PWM_RESOLUTION);

  if (!ok) {
    Serial.println("ERROR: PWM initialization failed!");
  }

#else

  ledcSetup(CH_LF, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(CH_LR, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(CH_RF, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(CH_RR, PWM_FREQUENCY, PWM_RESOLUTION);

  ledcAttachPin(LF_EN, CH_LF);
  ledcAttachPin(LR_EN, CH_LR);
  ledcAttachPin(RF_EN, CH_RF);
  ledcAttachPin(RR_EN, CH_RR);

#endif
}


// =====================================================
// PWM OUTPUT
// =====================================================

void writeMotorPWM(int pin, int channel, int pwm) {

  pwm = constrain(pwm, 0, MAX_PWM);

#if ESP_ARDUINO_VERSION_MAJOR >= 3

  ledcWrite(pin, pwm);

#else

  ledcWrite(channel, pwm);

#endif
}


// =====================================================
// INDIVIDUAL MOTOR CONTROL
// =====================================================

// speed: -255 to +255
// Positive and negative indicate opposite directions.

void setMotor(
  int enPin,
  int in1,
  int in2,
  int pwmChannel,
  int speed
) {

  speed = constrain(speed, -MAX_PWM, MAX_PWM);

  if (speed > 0) {

    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);

  } else if (speed < 0) {

    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);

  } else {

    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  writeMotorPWM(enPin, pwmChannel, abs(speed));
}


// =====================================================
// STOP ALL MOTORS
// =====================================================

void stopMotors() {

  currentLeftPWM = 0;
  currentRightPWM = 0;

  setMotor(LF_EN, LF_IN1, LF_IN2, CH_LF, 0);
  setMotor(LR_EN, LR_IN1, LR_IN2, CH_LR, 0);

  setMotor(RF_EN, RF_IN1, RF_IN2, CH_RF, 0);
  setMotor(RR_EN, RR_IN1, RR_IN2, CH_RR, 0);
}


// =====================================================
// LEFT AND RIGHT MOTOR CONTROL
// =====================================================

void setLeftMotors(int speed) {

  setMotor(LF_EN, LF_IN1, LF_IN2, CH_LF, speed);
  setMotor(LR_EN, LR_IN1, LR_IN2, CH_LR, speed);
}


void setRightMotors(int speed) {

  setMotor(RF_EN, RF_IN1, RF_IN2, CH_RF, speed);
  setMotor(RR_EN, RR_IN1, RR_IN2, CH_RR, speed);
}


// =====================================================
// JOYSTICK DEAD ZONE
// =====================================================

int applyDeadZone(int value) {

  if (abs(value) < JOYSTICK_DEAD_ZONE) {
    return 0;
  }

  return value;
}


// =====================================================
// ACCELERATION RAMP
// =====================================================

int rampValue(int current, int target) {

  if (current < target) {

    current += RAMP_STEP;

    if (current > target) {
      current = target;
    }

  } else if (current > target) {

    current -= RAMP_STEP;

    if (current < target) {
      current = target;
    }
  }

  return current;
}


// =====================================================
// CONTROLLER CONNECTION CALLBACKS
// =====================================================

void onConnectedController(ControllerPtr ctl) {

  bool accepted = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {

    if (myControllers[i] == nullptr) {

      myControllers[i] = ctl;
      accepted = true;

      Serial.println("Controller connected!");

      controllerConnected = true;
      emergencyStop = true;

      stopMotors();

      break;
    }
  }

  if (!accepted) {
    Serial.println("No controller slot available.");
  }
}


void onDisconnectedController(ControllerPtr ctl) {

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {

    if (myControllers[i] == ctl) {

      myControllers[i] = nullptr;

      Serial.println("Controller disconnected!");

      controllerConnected = false;
      emergencyStop = true;

      stopMotors();

      break;
    }
  }
}


// =====================================================
// GET ACTIVE CONTROLLER
// =====================================================

ControllerPtr getActiveController() {

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {

    if (myControllers[i] != nullptr &&
        myControllers[i]->isConnected()) {

      return myControllers[i];
    }
  }

  return nullptr;
}


// =====================================================
// MOTOR CONTROL
// =====================================================

void updateMotorControl(ControllerPtr ctl) {

  if (ctl == nullptr || !ctl->isConnected()) {

    stopMotors();
    emergencyStop = true;

    return;
  }

  // ---------------------------------------------------
  // EMERGENCY STOP
  // B generally corresponds to Circle on PlayStation-
  // style layouts, but mappings depend on the controller.
  // ---------------------------------------------------

  if (ctl->b()) {

    emergencyStop = true;

    stopMotors();

    return;
  }

  emergencyStop = false;


  // ---------------------------------------------------
  // SPEED MODES
  // L1 = slow
  // R1 = fast
  // Neither = normal
  // ---------------------------------------------------

  if (ctl->l1()) {

    speedLimit = SLOW_SPEED;

  } else if (ctl->r1()) {

    speedLimit = FAST_SPEED;

  } else {

    speedLimit = NORMAL_SPEED;
  }


  // ---------------------------------------------------
  // READ LEFT JOYSTICK
  // ---------------------------------------------------

  int rawThrottle = -ctl->axisY();
  int rawSteering =  ctl->axisX();

  rawThrottle = applyDeadZone(rawThrottle);
  rawSteering = applyDeadZone(rawSteering);


  // Bluepad32 joystick range is normally about -512 to 512.

  int throttle = constrain(
    rawThrottle * 100 / 512,
    -100,
    100
  );

  int steering = constrain(
    rawSteering * 100 / 512,
    -100,
    100
  );


  // ---------------------------------------------------
  // APPLY SPEED LIMIT
  // ---------------------------------------------------

  throttle = throttle * speedLimit / 100;
  steering = steering * speedLimit / 100;


  // ---------------------------------------------------
  // DIFFERENTIAL STEERING
  //
  // Forward + left:
  // Left side slows, right side speeds up.
  //
  // Steering with zero throttle:
  // Left and right sides rotate oppositely.
  // ---------------------------------------------------

  int leftTarget  = throttle + steering;
  int rightTarget = throttle - steering;


  // Preserve the requested steering ratio.

  int largest = max(
    abs(leftTarget),
    abs(rightTarget)
  );

  if (largest > 100) {

    leftTarget = leftTarget * 100 / largest;

    rightTarget = rightTarget * 100 / largest;
  }


  // ---------------------------------------------------
  // INDEPENDENT MOTOR CALIBRATION
  // ---------------------------------------------------

  leftTarget =
    leftTarget * leftCalibration / 100;

  rightTarget =
    rightTarget * rightCalibration / 100;

  leftTarget = constrain(leftTarget, -100, 100);

  rightTarget = constrain(rightTarget, -100, 100);


  // ---------------------------------------------------
  // CONVERT TO PWM
  // ---------------------------------------------------

  int targetLeftPWM =
    leftTarget * MAX_PWM / 100;

  int targetRightPWM =
    rightTarget * MAX_PWM / 100;


  // ---------------------------------------------------
  // ACCELERATION RAMP
  // ---------------------------------------------------

  unsigned long now = millis();

  if (now - lastRampTime >= RAMP_INTERVAL_MS) {

    lastRampTime = now;

    currentLeftPWM = rampValue(
      currentLeftPWM,
      targetLeftPWM
    );

    currentRightPWM = rampValue(
      currentRightPWM,
      targetRightPWM
    );
  }


  // ---------------------------------------------------
  // OUTPUT TO ALL FOUR MOTORS
  // ---------------------------------------------------

  setLeftMotors(currentLeftPWM);

  setRightMotors(currentRightPWM);
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP32 OBSTACLE-RACING CAR");
  Serial.println("==============================");


  // Initialize motor direction pins.

  pinMode(LF_IN1, OUTPUT);
  pinMode(LF_IN2, OUTPUT);

  pinMode(LR_IN1, OUTPUT);
  pinMode(LR_IN2, OUTPUT);

  pinMode(RF_IN1, OUTPUT);
  pinMode(RF_IN2, OUTPUT);

  pinMode(RR_IN1, OUTPUT);
  pinMode(RR_IN2, OUTPUT);


  // Ensure motor direction pins start LOW.

  digitalWrite(LF_IN1, LOW);
  digitalWrite(LF_IN2, LOW);

  digitalWrite(LR_IN1, LOW);
  digitalWrite(LR_IN2, LOW);

  digitalWrite(RF_IN1, LOW);
  digitalWrite(RF_IN2, LOW);

  digitalWrite(RR_IN1, LOW);
  digitalWrite(RR_IN2, LOW);


  // Initialize PWM.

  setupPWM();

  stopMotors();


  // Initialize controller slots.

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {

    myControllers[i] = nullptr;
  }


  // Initialize Bluepad32.

  BP32.setup(
    &onConnectedController,
    &onDisconnectedController
  );

  // Clear previous Bluetooth bonding information.
  // If pairing fails repeatedly, remove this line
  // and re-upload to preserve existing bonds.
  BP32.forgetBluetoothKeys();

  Serial.println("Waiting for Bluetooth controller...");
  Serial.println("Pair the EvoFox controller now.");
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // Process Bluetooth controller events.

  BP32.update();


  ControllerPtr ctl = getActiveController();


  // Stop if there is no connected controller.

  if (ctl == nullptr) {

    controllerConnected = false;

    stopMotors();

    delay(5);

    return;
  }

  controllerConnected = true;


  // Update motor commands.

  updateMotorControl(ctl);


  delay(1);
}