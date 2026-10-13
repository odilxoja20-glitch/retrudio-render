#include <LiquidCrystal.h>

// DRONE PROPULSION AND POWER PROTECTION TESTBENCH

// Analog sensor pins
const int THROTTLE_PIN = A0;
const int BATTERY_PIN = A1;
const int LEFT_TEMP_PIN = A2;
const int RIGHT_TEMP_PIN = A3;

// Motor PWM pins
const int LEFT_MOTOR_PIN = 5;
const int RIGHT_MOTOR_PIN = 6;

// Status indicators
const int GREEN_LED = 13;
const int YELLOW_LED = 2;
const int RED_LED = 4;
const int BUZZER_PIN = 3;

// LCD pins
// LiquidCrystal(RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// BATTERY SETTINGS

// Simulated three-cell lithium battery voltage range
const float MIN_SIMULATED_VOLTAGE = 9.0;
const float MAX_SIMULATED_VOLTAGE = 12.6;

// Low-battery warning threshold
const float LOW_BATTERY_VOLTAGE = 10.8;

// Critical-battery shutdown threshold
const float CRITICAL_BATTERY_VOLTAGE = 9.9;

// Hysteresis values help prevent rapidly switching modes
const float LOW_BATTERY_RECOVERY = 11.1;
const float CRITICAL_RECOVERY = 10.2;

// TEMPERATURE SETTINGS

// Motor output begins to be limited at this temperature
const float DERATE_TEMPERATURE = 45.0;

// Motors shut down at this temperature
const float SHUTDOWN_TEMPERATURE = 60.0;

// Temperature must fall below this value to recover
const float RECOVERY_TEMPERATURE = 55.0;

// MOTOR SETTINGS

// Maximum Arduino PWM value
const int MAX_PWM = 255;

// Maximum output allowed during a warning
const int DERATED_PWM = 150;

// Maximum PWM change during each program cycle
// This creates a soft-start and soft-stop effect.
const int PWM_STEP = 4;

// TIMING SETTINGS

const unsigned long SENSOR_INTERVAL = 100;
const unsigned long DISPLAY_INTERVAL = 500;
const unsigned long BUZZER_INTERVAL = 500;

// SYSTEM STATES

enum SystemState
{
  NORMAL,
  LOW_BATTERY,
  THERMAL_DERATING,
  CRITICAL_FAULT
};

SystemState systemState = NORMAL;

// GLOBAL VARIABLES

int throttleRaw = 0;
int requestedPWM = 0;
int targetPWM = 0;
int currentPWM = 0;

float batteryVoltage = 0.0;
float leftTemperature = 0.0;
float rightTemperature = 0.0;
float highestTemperature = 0.0;

bool buzzerOn = false;

unsigned long previousSensorTime = 0;
unsigned long previousDisplayTime = 0;
unsigned long previousBuzzerTime = 0;

// SETUP

void setup()
{
  // Configure motor pins as outputs
  pinMode(LEFT_MOTOR_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN, OUTPUT);

  // Configure status indicators as outputs
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Analog pins are inputs by default, but declaring them
  // makes the intended purpose of each pin clearer.
  pinMode(THROTTLE_PIN, INPUT);
  pinMode(BATTERY_PIN, INPUT);
  pinMode(LEFT_TEMP_PIN, INPUT);
  pinMode(RIGHT_TEMP_PIN, INPUT);

  // Ensure motors and alarms are off during startup
  analogWrite(LEFT_MOTOR_PIN, 0);
  analogWrite(RIGHT_MOTOR_PIN, 0);
  noTone(BUZZER_PIN);

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("DRONE POWER SYS");

  lcd.setCursor(0, 1);
  lcd.print("INITIALIZING...");

  // Initialize Serial Monitor for data logging
  Serial.begin(9600);

  delay(2000);
  lcd.clear();
}

// MAIN LOOP

void loop()
{
  unsigned long currentTime = millis();

  // Read sensors every 100 milliseconds
  if (currentTime - previousSensorTime >= SENSOR_INTERVAL)
  {
    previousSensorTime = currentTime;

    readSensors();
    determineSystemState();
    calculateMotorOutput();
    updateMotorSpeed();
    updateStatusIndicators();
    printSerialData();
  }

  // Update the LCD every 500 milliseconds
  if (currentTime - previousDisplayTime >= DISPLAY_INTERVAL)
  {
    previousDisplayTime = currentTime;
    updateLCD();
  }

  // Control warning buzzer without blocking the program
  updateBuzzer(currentTime);
}

// SENSOR READING FUNCTIONS

void readSensors()
{
  // Read the pilot throttle command
  throttleRaw = analogRead(THROTTLE_PIN);

  // Convert throttle from 0-1023 to PWM 0-255
  requestedPWM = map(throttleRaw, 0, 1023, 0, MAX_PWM);

  // Read simulated battery voltage
  batteryVoltage = readBatteryVoltage();

  // Read both TMP36 temperature sensors
  leftTemperature = readTMP36(LEFT_TEMP_PIN);
  rightTemperature = readTMP36(RIGHT_TEMP_PIN);

  // Determine which motor temperature is higher
  highestTemperature = max(leftTemperature, rightTemperature);
}

// Read and average the battery potentiometer
float readBatteryVoltage()
{
  long total = 0;

  // Average ten readings to reduce measurement noise
  for (int i = 0; i < 10; i++)
  {
    total += analogRead(BATTERY_PIN);
  }

  float averageReading = total / 10.0;

  // Convert potentiometer reading to simulated 9.0-12.6 V
  float voltage =
      MIN_SIMULATED_VOLTAGE +
      (averageReading / 1023.0) *
      (MAX_SIMULATED_VOLTAGE - MIN_SIMULATED_VOLTAGE);

  return voltage;
}

// Read and convert a TMP36 sensor value to degrees Celsius
float readTMP36(int sensorPin)
{
  long total = 0;

  // Average ten readings for a more stable temperature value
  for (int i = 0; i < 10; i++)
  {
    total += analogRead(sensorPin);
  }

  float averageReading = total / 10.0;

  // Convert Arduino reading into voltage
  float sensorVoltage = averageReading * (5.0 / 1023.0);

  // TMP36 output is 0.5 V at 0 degrees Celsius
  // It increases by 0.01 V per degree Celsius.
  float temperatureC = (sensorVoltage - 0.5) * 100.0;

  return temperatureC;
}

// STATE-MACHINE LOGIC

void determineSystemState()
{
  // Critical faults have the highest priority.
  if (batteryVoltage <= CRITICAL_BATTERY_VOLTAGE ||
      highestTemperature >= SHUTDOWN_TEMPERATURE)
  {
    systemState = CRITICAL_FAULT;
    return;
  }

  // Remain in critical mode until conditions recover enough.
  if (systemState == CRITICAL_FAULT)
  {
    bool batteryRecovered =
        batteryVoltage > CRITICAL_RECOVERY;

    bool temperatureRecovered =
        highestTemperature < RECOVERY_TEMPERATURE;

    if (!batteryRecovered || !temperatureRecovered)
    {
      return;
    }
  }

  // Apply thermal derating when either motor becomes hot.
  if (highestTemperature >= DERATE_TEMPERATURE)
  {
    systemState = THERMAL_DERATING;
  }

  // Apply low-battery derating.
  else if (batteryVoltage <= LOW_BATTERY_VOLTAGE)
  {
    systemState = LOW_BATTERY;
  }

  // Use recovery hysteresis before returning to normal.
  else if (systemState == LOW_BATTERY &&
           batteryVoltage < LOW_BATTERY_RECOVERY)
  {
    systemState = LOW_BATTERY;
  }

  else
  {
    systemState = NORMAL;
  }
}

// MOTOR CONTROL

void calculateMotorOutput()
{
  switch (systemState)
  {
    case NORMAL:
      // Normal mode allows full requested throttle.
      targetPWM = requestedPWM;
      break;

    case LOW_BATTERY:
      // Limit motor output to reduce battery demand.
      targetPWM = min(requestedPWM, DERATED_PWM);
      break;

    case THERMAL_DERATING:
      // Limit output to reduce heating.
      targetPWM = min(requestedPWM, DERATED_PWM);
      break;

    case CRITICAL_FAULT:
      // Shut both motors off.
      targetPWM = 0;
      break;
  }
}

void updateMotorSpeed()
{
  // Gradually increase motor speed toward the target.
  if (currentPWM < targetPWM)
  {
    currentPWM += PWM_STEP;

    if (currentPWM > targetPWM)
    {
      currentPWM = targetPWM;
    }
  }

  // Gradually decrease motor speed toward the target.
  else if (currentPWM > targetPWM)
  {
    currentPWM -= PWM_STEP;

    if (currentPWM < targetPWM)
    {
      currentPWM = targetPWM;
    }
  }

  // Apply the same output to both motors.
  analogWrite(LEFT_MOTOR_PIN, currentPWM);
  analogWrite(RIGHT_MOTOR_PIN, currentPWM);
}

// LED STATUS INDICATORS

void updateStatusIndicators()
{
  // Turn all LEDs off before selecting the active one.
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  switch (systemState)
  {
    case NORMAL:
      digitalWrite(GREEN_LED, HIGH);
      break;

    case LOW_BATTERY:
    case THERMAL_DERATING:
      digitalWrite(YELLOW_LED, HIGH);
      break;

    case CRITICAL_FAULT:
      digitalWrite(RED_LED, HIGH);
      break;
  }
}

// BUZZER CONTROL

void updateBuzzer(unsigned long currentTime)
{
  // No buzzer in normal mode
  if (systemState == NORMAL)
  {
    noTone(BUZZER_PIN);
    buzzerOn = false;
    return;
  }

  // Toggle the buzzer at regular intervals
  if (currentTime - previousBuzzerTime >= BUZZER_INTERVAL)
  {
    previousBuzzerTime = currentTime;
    buzzerOn = !buzzerOn;

    if (buzzerOn)
    {
      if (systemState == CRITICAL_FAULT)
      {
        // Higher frequency for critical fault
        tone(BUZZER_PIN, 1200);
      }
      else
      {
        // Lower frequency for warnings
        tone(BUZZER_PIN, 700);
      }
    }
    else
    {
      noTone(BUZZER_PIN);
    }
  }
}

// LCD DISPLAY

void updateLCD()
{
  lcd.clear();

  // First LCD row: battery and throttle
  lcd.setCursor(0, 0);
  lcd.print("B:");
  lcd.print(batteryVoltage, 1);
  lcd.print("V ");

  lcd.print("T:");
  int throttlePercent = map(currentPWM, 0, 255, 0, 100);
  lcd.print(throttlePercent);
  lcd.print("%");

  // Second LCD row: operating state
  lcd.setCursor(0, 1);

  switch (systemState)
  {
    case NORMAL:
      lcd.print("SYSTEM NORMAL");
      break;

    case LOW_BATTERY:
      lcd.print("LOW BATTERY");
      break;

    case THERMAL_DERATING:
      lcd.print("THERMAL LIMIT");
      break;

    case CRITICAL_FAULT:
      if (batteryVoltage <= CRITICAL_BATTERY_VOLTAGE)
      {
        lcd.print("BATTERY FAULT");
      }
      else
      {
        lcd.print("TEMP SHUTDOWN");
      }
      break;
  }
}

// SERIAL MONITOR DATA LOGGING

void printSerialData()
{
  Serial.print("Throttle Raw: ");
  Serial.print(throttleRaw);

  Serial.print(" | PWM: ");
  Serial.print(currentPWM);

  Serial.print(" | Battery: ");
  Serial.print(batteryVoltage, 2);
  Serial.print(" V");

  Serial.print(" | Left Temp: ");
  Serial.print(leftTemperature, 1);
  Serial.print(" C");

  Serial.print(" | Right Temp: ");
  Serial.print(rightTemperature, 1);
  Serial.print(" C");

  Serial.print(" | State: ");

  switch (systemState)
  {
    case NORMAL:
      Serial.println("NORMAL");
      break;

    case LOW_BATTERY:
      Serial.println("LOW BATTERY");
      break;

    case THERMAL_DERATING:
      Serial.println("THERMAL DERATING");
      break;

    case CRITICAL_FAULT:
      Serial.println("CRITICAL FAULT");
      break;
  }
}