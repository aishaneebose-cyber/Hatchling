#include <Arduino.h>
#include <TurtleReceiver.h>
#include <ESP32Servo.h>

NetController controller(false);

// ==========================================
// PIN DEFINITIONS  (all unique now!)
// ==========================================
// Left Drive Motor
const int LEFT_MOTOR_EN  = 14;   // ENA  -> driven HIGH to enable
const int LEFT_MOTOR_IN1 = 32;
const int LEFT_MOTOR_IN2 = 33;

// Right Drive Motor
const int RIGHT_MOTOR_EN  = 25;  // ENB  -> CHANGED: was 32 (conflict). Use a free pin.
const int RIGHT_MOTOR_IN1 = 26;
const int RIGHT_MOTOR_IN2 = 27;

// Elevator Pulley Motor
const int ELEVATOR_EN  = 18;     // CHANGED: enable on its own pin
const int ELEVATOR_IN1 = 19;
const int ELEVATOR_IN2 = 23;     // CHANGED: was 18 (conflict). Use a free pin.

// Claw Servo
const int CLAW_SERVO_PIN = 5;

// ==========================================
// CLAW TUNING
// ==========================================
const int CLAW_OPEN_ANGLE = 150;
const int CLAW_CLOSED_ANGLE = 30;
const int CLAW_STEP_DEG = 2;
const int SG90_MIN_PULSE_US = 1000;
const int SG90_MAX_PULSE_US = 2000;

Servo clawServo;
int clawAngle = CLAW_OPEN_ANGLE;

// Digital-only motor control: direction = +1 (fwd), -1 (rev), 0 (stop)
void setMotor(int enPin, int in1Pin, int in2Pin, int direction) {
  if (direction > 0) {
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
    digitalWrite(enPin, HIGH);   // enable the H-bridge (full speed)
  } else if (direction < 0) {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
    digitalWrite(enPin, HIGH);
  } else {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, LOW);
    digitalWrite(enPin, LOW);    // disable / coast
  }
}

void moveClaw(int direction) {
  int minAngle = min(CLAW_OPEN_ANGLE, CLAW_CLOSED_ANGLE);
  int maxAngle = max(CLAW_OPEN_ANGLE, CLAW_CLOSED_ANGLE);
  int closeDir = (CLAW_CLOSED_ANGLE > CLAW_OPEN_ANGLE) ? 1 : -1;

  clawAngle += direction * closeDir * CLAW_STEP_DEG;
  clawAngle = constrain(clawAngle, minAngle, maxAngle);
  clawServo.write(clawAngle);
}

void setup() {
  Serial.begin(115200);

  int motorPins[] = {
    LEFT_MOTOR_EN, LEFT_MOTOR_IN1, LEFT_MOTOR_IN2,
    RIGHT_MOTOR_EN, RIGHT_MOTOR_IN1, RIGHT_MOTOR_IN2,
    ELEVATOR_EN, ELEVATOR_IN1, ELEVATOR_IN2
  };
  for (int pin : motorPins) {
    pinMode(pin, OUTPUT);
  }

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  clawServo.setPeriodHertz(50);
  clawServo.attach(CLAW_SERVO_PIN, SG90_MIN_PULSE_US, SG90_MAX_PULSE_US);
  clawServo.write(clawAngle);

  printMacAddress();
  controller.controllerSetup();
}

void loop() {
  // ---- BUTTON DRIVE ----
  int leftDir = 0;
  int rightDir = 0;

  if (controller.getY()) {          // FRONT: both forward
    leftDir = 1;  rightDir = 1;
  } else if (controller.getA()) {   // BACK: both backward
    leftDir = -1; rightDir = -1;
  } else if (controller.getX()) {   // LEFT wheel only
    leftDir = 1;  rightDir = 0;
  } else if (controller.getB()) {   // RIGHT wheel only
    leftDir = 0;  rightDir = 1;
  }

  setMotor(LEFT_MOTOR_EN, LEFT_MOTOR_IN1, LEFT_MOTOR_IN2, leftDir);
  setMotor(RIGHT_MOTOR_EN, RIGHT_MOTOR_IN1, RIGHT_MOTOR_IN2, rightDir);

  // ---- ELEVATOR ----
  if (controller.getRb()) {
    setMotor(ELEVATOR_EN, ELEVATOR_IN1, ELEVATOR_IN2, 1);
  } else if (controller.getRt()) {
    setMotor(ELEVATOR_EN, ELEVATOR_IN1, ELEVATOR_IN2, -1);
  } else {
    setMotor(ELEVATOR_EN, ELEVATOR_IN1, ELEVATOR_IN2, 0);
  }

  // ---- CLAW ----
  if (controller.getLb()) {
    moveClaw(+1);
  } else if (controller.getLt()) {
    moveClaw(-1);
  }

  delay(20);
}
