#include <LiquidCrystal.h>

// Pins for sensors and power gating
const int SOIL_POWER = A0;   // Controls power to sensor
const int SOIL_SENSOR = A1;  // Moisture reading
const int LIGHT_SENSOR = A2; // Light level reading

// Pins for Actuators
const int MOTOR_PIN = 5;    // DC Motor (Pump)
const int BLUE_LED = 6;     // Watering Indicator
const int RED_LED = 4;      // Status LEDs
const int YELLOW_LED = 3;
const int GREEN_LED = 2;

// Pins for LCD
const int RS = 7, E = 8, DB4 = 9, DB5 = 10, DB6 = 11, DB7 = 12;
LiquidCrystal lcd(RS, E, DB4, DB5, DB6, DB7);

int moisture = 0;
int lightLevel = 0;

void setup() {
  Serial.begin(9600);
  pinMode(SOIL_SENSOR, INPUT);
  pinMode(LIGHT_SENSOR, INPUT);
  
  pinMode(SOIL_POWER, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  
  lcd.begin(16, 2);
  lcd.print("Plant System...");
  delay(2000);
  lcd.clear();
}

void loop() {
  // 1. Power Gating: Turn sensor on, wait, read, turn off
  digitalWrite(SOIL_POWER, HIGH);
  delay(10);
  moisture = analogRead(SOIL_SENSOR);
  lightLevel = analogRead(LIGHT_SENSOR);
  digitalWrite(SOIL_POWER, LOW);
  Serial.print("Moisture = ");
  Serial.println(moisture);
  Serial.print("LightLevel = ");
  Serial.println(lightLevel);

  // 2. Watering Logic
  // Only water if it's "daytime" (light sensor > 500) and soil is dry (< 200)
  if (moisture < 200 && lightLevel > 500) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Watering...");
    
    digitalWrite(MOTOR_PIN, HIGH);
    
    // Blink LED and run motor for 5 seconds (25 loops of 200ms)
    for (int i = 0; i < 25; i++) {
      digitalWrite(BLUE_LED, HIGH);
      
      // SIMULATION: Calculate a "fake" moisture that rises
      // 200 is the starting dry value, it increases by ~20 each step
      int fakeMoisture = 10 + (i * 20); 
      
      lcd.setCursor(10, 0);
      lcd.print(fakeMoisture);
      lcd.print("  ");

      delay(100);
      digitalWrite(BLUE_LED, LOW);
      delay(100);
    }
    digitalWrite(MOTOR_PIN, LOW);
  }

  // 3. Status Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moist: ");
  lcd.print(moisture);
  
  lcd.setCursor(0, 1);
  lcd.print("Water: ");

  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  if (moisture < 200) {
    lcd.print("CRITICAL");
    digitalWrite(RED_LED, HIGH);
  } else if (moisture < 600) {
    lcd.print("NEEDED");
    digitalWrite(YELLOW_LED, HIGH);
  } else {
    lcd.print("OK");
    digitalWrite(GREEN_LED, HIGH);
  }

  delay(1000);
}