#include <Servo.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
Servo panelServo;

int topLeftPin = A0;
int topRightPin = A1;
int bottomLeftPin = A2;
int bottomRightPin = A3;
int solarPin = A4;

int topLeftValue, topRightValue, bottomLeftValue, bottomRightValue;
int topAvg, bottomAvg;
int solarValue;
float solarVoltage;

int deadband = 30;
int currentAngle = 90;

unsigned long lastMoveTime = 0;
int moveInterval = 30;

int faultThreshold = 150;

void setup() {
  Serial.begin(9600);
  panelServo.attach(9);
  panelServo.write(currentAngle);
  lcd.begin(16, 2);
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(topAvg);
  lcd.print(" B:");
  lcd.print(bottomAvg);

  lcd.setCursor(0, 1);
  lcd.print("V:");
  lcd.print(solarVoltage, 2);
  lcd.print(" A:");
  lcd.print(currentAngle);
}

void loop() {
  topLeftValue = analogRead(topLeftPin);
  topRightValue = analogRead(topRightPin);
  bottomLeftValue = analogRead(bottomLeftPin);
  bottomRightValue = analogRead(bottomRightPin);

  topAvg = (topLeftValue + topRightValue) / 2;
  bottomAvg = (bottomLeftValue + bottomRightValue) / 2;

  solarValue = analogRead(solarPin);
  solarVoltage = solarValue * (5.0 / 1023.0);

  int difference = topAvg - bottomAvg;

  bool topFault = abs(topLeftValue - topRightValue) > faultThreshold;
  bool bottomFault = abs(bottomLeftValue - bottomRightValue) > faultThreshold;

  if (millis() - lastMoveTime >= moveInterval) {
    if (difference > deadband) {
      if (currentAngle > 0) {
        currentAngle--;
        panelServo.write(currentAngle);
      }
    } else if (difference < -deadband) {
      if (currentAngle < 180) {
        currentAngle++;
        panelServo.write(currentAngle);
      }
    }
    lastMoveTime = millis();
  }

  Serial.print("T(avg): "); Serial.print(topAvg);
  Serial.print("  B(avg): "); Serial.print(bottomAvg);
  Serial.print("  Solar: "); Serial.print(solarVoltage);
  Serial.print("V  Angle: "); Serial.print(currentAngle);
  if (topFault) Serial.print("  [TOP FAULT]");
  if (bottomFault) Serial.print("  [BOTTOM FAULT]");
  Serial.println();

  updateLCD();
}