#include <Servo.h>
#include <LiquidCrystal.h>

// =====================================================
// LCD
// =====================================================
LiquidCrystal lcd(8, 7, 6, 5, 4, 2);

// =====================================================
// SERVO
// =====================================================
Servo radarServo;

// =====================================================
// RGB LED PINS
// =====================================================
const int RED = 11;
const int GREEN = 9;
const int BLUE = 10;

// =====================================================
// ULTRASONIC SENSOR
// =====================================================
const int pingPin = A1;

// =====================================================
// LDR SENSOR
// =====================================================
const int LDR = A0;

// =====================================================
// SETUP
// =====================================================
void setup() {

  lcd.begin(16, 2);

  lcd.setCursor(0,0);
  lcd.print("Adaptive Radar");

  delay(2000);

  lcd.clear();

  radarServo.attach(3);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  for (int angle = 0; angle <= 180; angle += 2) {

    radarSweep(angle);

  }

  for (int angle = 180; angle >= 0; angle -= 2) {

    radarSweep(angle);

  }
}

// =====================================================
// RADAR FUNCTION
// =====================================================
void radarSweep(int angle) {

  radarServo.write(angle);

  delay(20);

  int distance = getDistance();

  int lightValue = analogRead(LDR);

  // RGB COLORS
  if (distance <= 40) {

    setRGB(255, 0, 0);

  }
  else if (distance <= 80) {

    setRGB(0, 255, 0);

  }
  else {

    setRGB(0, 0, 255);

  }

  // LCD DISPLAY
  lcd.setCursor(0,0);
  lcd.print("A:");
  lcd.print(angle);
  lcd.print("   ");

  lcd.setCursor(0,1);
  lcd.print("D:");
  lcd.print(distance);
  lcd.print(" ");

  lcd.print("L:");
  lcd.print(lightValue);
  lcd.print("   ");

  // LDR SPEED CONTROL
  if (lightValue < 300) {

    delay(10);

  }
  else {

    delay(40);

  }
}

// =====================================================
// ULTRASONIC FUNCTION
// =====================================================
int getDistance() {

  long total = 0;

  for (int i = 0; i < 5; i++) {

    pinMode(pingPin, OUTPUT);

    digitalWrite(pingPin, LOW);
    delayMicroseconds(2);

    digitalWrite(pingPin, HIGH);
    delayMicroseconds(5);

    digitalWrite(pingPin, LOW);

    pinMode(pingPin, INPUT);

    long duration = pulseIn(pingPin, HIGH);

    int distance = duration * 0.034 / 2;

    total += distance;

    delay(10);
  }

  return total / 5;
}

// =====================================================
// RGB FUNCTION
// =====================================================
void setRGB(int r, int g, int b) {

  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
}
