#include <LiquidCrystal.h>
#include <Adafruit_NeoPixel.h>
// C++ code
//

// LCD Constants
const int RS  = 2;
const int E   = 3;
const int DB4 = 4;
const int DB5 = 5;
const int DB6 = 6;
const int DB7 = 7;
const int DC_MOTOR = 9;

// DC MOTOR Constants
const int OFF_SPEED = 0;
const int LOW_SPEED = 64;
const int MED_SPEED = 128;
const int HIGH_SPEED = 255;

// Thresholds
const int TEMP_LOW_THRESHOLD = 65;
const int TEMP_HIGH_THRESHOLD = 80;
const int FAN_LOW_THRESHOLD = 80;
const int FAN_MED_THRESHOLD = 90;
const int FAN_HIGH_THRESHOLD = 100;
const int WATER_LOW_THRESHOLD = 300;

// Create LCD object
LiquidCrystal lcd(RS, E, DB4, DB5, DB6, DB7);

// Soil moisture sensor
const int MOISTURE_SENSOR = A0;

// TMP36 temperature sensor
const int TEMP_SENSOR = A1;

// NeoPixel PIN
const int PIXEL_PIN = 8;

// Pixel Count
const int PIXEL_COUNT = 32;


// Create NeoPixel strip object
Adafruit_NeoPixel strip(
  PIXEL_COUNT,
  PIXEL_PIN,
  NEO_GRB + NEO_KHZ800
);

// Colors
const uint32_t RED = strip.Color(255, 0, 0);
const uint32_t GREEN = strip.Color(0, 255, 0);
const uint32_t OFF = strip.Color(0, 0, 0);

// Variables
int moistureValue = 0;
int tempSensorValue = 0;
int tempC = 0;
int tempF = 0;


void setup()
{
  // Temp Sensor
  pinMode(TEMP_SENSOR, INPUT);
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.begin(16, 2);
    
  // Initialize NeoPixels
  strip.begin();
  strip.show();
  
  // DC_MOTOR
  pinMode(DC_MOTOR, OUTPUT);
  
  // Startup message
  lcd.setCursor(0, 0);
  lcd.print("Quail Monitor");
  delay(1500);
}

void loop()
{
  // Read the sensor value (analog)
  tempSensorValue = analogRead(TEMP_SENSOR);
  
  // Map sensor value (20 to 358) to celsius temp (-40 to 125)
  tempC = map(tempSensorValue, 20, 358, -40, 125);
  
  // Calculate the temp in Fahrenheit
  tempF = 9 * tempC / 5 + 32;
  Serial.print("Temp Value: ");
  Serial.println(tempF);
  
  // Read moisture sensor
  moistureValue = analogRead(MOISTURE_SENSOR);
  Serial.print("Moisture Value: ");
  Serial.println(moistureValue);
    
  // Determine warning states
  bool tempLow = tempF < TEMP_LOW_THRESHOLD;
  bool tempHigh = tempF >= TEMP_HIGH_THRESHOLD;
  bool waterLow = moistureValue < WATER_LOW_THRESHOLD;
  
  // Determine overall system status
  bool systemSafe = !tempLow && !tempHigh && !waterLow;
  
  // Updating NeoPixel to reflect status
  if (systemSafe) {
    setNeoColor(GREEN);
  }
  else {
    setNeoColor(RED);
  }
  
  // Update fan speed
  updateFanSpeed();
    
  // Always show temperature first
  showTemperature();
  delay(2000);
  
  // Rotate warning messages
  if (tempLow) {
    showWarning("TEMP LOW");
    delay(2000);
  }

  if (tempHigh) {
    showWarning("TEMP HIGH");
    delay(2000);
  }

  if (waterLow) {
    showWarning("WATER LOW");
    delay(2000);
  }
  
  // If safe, show SAFE message
  if (systemSafe) {
    showSafe();
    delay(2000);
  }
  
}

// Helper functions
void setNeoColor(uint32_t color){
  for (int i = 0; i < PIXEL_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void showTemperature() {
  clearLCDRows();
  lcd.setCursor(0, 0);
  lcd.print("Temperature:");
  lcd.setCursor(0, 1);
  lcd.print(tempF);
  lcd.print(" F");
}

void showWarning(String message) {
  clearLCDRows();
  lcd.setCursor(0, 0);
  lcd.print("WARNING:");
  lcd.setCursor(0, 1);
  lcd.print(message);
}

void showSafe() {
  clearLCDRows();
  lcd.setCursor(0, 0);
  lcd.print("System Status");
  lcd.setCursor(0, 1);
  lcd.print("ALL GOOD");
}

void clearLCDRows() {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void updateFanSpeed() {
  if (tempF >= FAN_HIGH_THRESHOLD) {
    // Very hot
    analogWrite(DC_MOTOR, HIGH_SPEED);
  }
  else if (tempF >= FAN_MED_THRESHOLD) {
    // Hot
    analogWrite(DC_MOTOR, MED_SPEED);
  }
  else if (tempF >= FAN_LOW_THRESHOLD) {
	// Warm
    analogWrite(DC_MOTOR, LOW_SPEED);
  }
  else {
	// Safe
    analogWrite(DC_MOTOR, OFF_SPEED);
  }
}