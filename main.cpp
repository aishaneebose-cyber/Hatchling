#include <Arduino.h>
#include <TurtleReceiver.h>

// Initialize HURC NetController (false = disable serial debug spam)
NetController controller(false);

// ==========================================
// PIN DEFINITIONS (Update to match your wiring)
// ==========================================
// Left Drive Motor (Controls the 2 left wheels)
const int LEFT_MOTOR_PWM = 14; //speed control
const int LEFT_MOTOR_IN1 = 20; //pins for motor
const int LEFT_MOTOR_IN2 = 21; //pin for motor

// Right Drive Motor (Controls the 2 right wheels)
const int RIGHT_MOTOR_PWM = 32; //speed control
const int RIGHT_MOTOR_IN1 = 22; //pins for motor
const int RIGHT_MOTOR_IN2 = 23; //pins for motor

// Elevator Pulley Motor (Spins rod/string up and down)
const int ELEVATOR_PWM = 18;
const int ELEVATOR_IN1 = 19;
const int ELEVATOR_IN2 = 21;

// Claw Motor (Opens and closes the claw)
// officer recommended using servo instead
const int CLAW_PWM = 4;
const int CLAW_IN1 = 16;
const int CLAW_IN2 = 17;

// ==========================================
// TUNING & SPEED SETTINGS (0 to 255)
// ==========================================
const float JOYSTICK_DEADZONE = 0.10; // Ignores tiny stick drift
const int ELEVATOR_SPEED = 200;       // Speed for lifting/lowering
const int CLAW_SPEED = 180;           // Speed for opening/closing claw

// Helper function to drive standard DC motors (L298N / TB6612FNG style)
void setMotor(int pwmPin, int in1Pin, int in2Pin, int speed) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
    analogWrite(pwmPin, speed);
  } else if (speed < 0) {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
    analogWrite(pwmPin, abs(speed));
  } else {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, LOW);
    analogWrite(pwmPin, 0);
  }
}

void setup() {
  Serial.begin(115200);

  // Set all motor control pins as outputs
  int motorPins[] = {
    LEFT_MOTOR_PWM, LEFT_MOTOR_IN1, LEFT_MOTOR_IN2,
    RIGHT_MOTOR_PWM, RIGHT_MOTOR_IN1, RIGHT_MOTOR_IN2,
    ELEVATOR_PWM, ELEVATOR_IN1, ELEVATOR_IN2,
    CLAW_PWM, CLAW_IN1, CLAW_IN2
  };

  for (int pin : motorPins) {
    pinMode(pin, OUTPUT);
  }

  // Required: Initialize HURC controller receiver
  controller.controllerSetup();
}

void loop() {
  // ------------------------------------------
  // 1. TANK DRIVE (Left Stick = Left Wheels, Right Stick = Right Wheels)
  // ------------------------------------------
  float leftInput = controller.getJoy1Y();
  float rightInput = controller.getJoy2Y();

  // Apply deadzone to prevent motor humming when sticks are centered
  if (abs(leftInput) < JOYSTICK_DEADZONE) leftInput = 0;
  if (abs(rightInput) < JOYSTICK_DEADZONE) rightInput = 0;

  // Map [-1.0, 1.0] joystick float to [-255, 255] motor speed
  int leftSpeed = (int)(leftInput * 255);
  int rightSpeed = (int)(rightInput * 255);

  setMotor(LEFT_MOTOR_PWM, LEFT_MOTOR_IN1, LEFT_MOTOR_IN2, leftSpeed);
  setMotor(RIGHT_MOTOR_PWM, RIGHT_MOTOR_IN1, RIGHT_MOTOR_IN2, rightSpeed);

  // ------------------------------------------
  // 2. ELEVATOR PULLEY SYSTEM (Right Bumper = Up, Right Trigger = Down)
  // ------------------------------------------
  if (controller.getRb()) {
    setMotor(ELEVATOR_PWM, ELEVATOR_IN1, ELEVATOR_IN2, ELEVATOR_SPEED);
  } else if (controller.getRt()) {
    setMotor(ELEVATOR_PWM, ELEVATOR_IN1, ELEVATOR_IN2, -ELEVATOR_SPEED);
  } else {
    setMotor(ELEVATOR_PWM, ELEVATOR_IN1, ELEVATOR_IN2, 0);
  }

  // ------------------------------------------
  // 3. CLAW CONTROL (Left Bumper = Close, Left Trigger = Open)
  // ------------------------------------------
  if (controller.getLb()) {
    setMotor(CLAW_PWM, CLAW_IN1, CLAW_IN2, CLAW_SPEED);
  } else if (controller.getLt()) {
    setMotor(CLAW_PWM, CLAW_IN1, CLAW_IN2, -CLAW_SPEED);
  } else {
    setMotor(CLAW_PWM, CLAW_IN1, CLAW_IN2, 0);
  }

  delay(20); // Small loop delay for stability
}
