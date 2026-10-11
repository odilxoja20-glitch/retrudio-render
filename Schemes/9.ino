#include <LiquidCrystal.h> 
#include <Servo.h>

//definitions
#define RAIN_SENSOR   A0   // analog input from Flex sensor
#define TEMP_SENSOR   A1   // analog input from temperature sensor
#define SERVO_PIN     3    // for the retraction servo motor
#define BUZZER        9    // for the alarm buzzer

//LCD parallel matrix RS, E, D4, D5, D6, D7
const int rs = 12, en = 11, d4 = 7, d5 = 5, d6 = 4, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//constants 
const int RAIN_THRESHOLD    = 102; //rain detection
const int EXTENDED_POSITION  = 0;   // when 0 degrees clothes can stay outside
const int RETRACTED_POSITION = 180; // 180degree> clothes will get retracted

//timing the variables
unsigned long previousMillis = 0;  //stores last time screen updated
const long interval          = 1000; //screen refresh

Servo winchServo;                   

void setup() {
    pinMode(RAIN_SENSOR, INPUT);
    pinMode(TEMP_SENSOR, INPUT);
    pinMode(BUZZER, OUTPUT);
    
    winchServo.attach(SERVO_PIN);
    winchServo.write(EXTENDED_POSITION); 
    
    Serial.begin(9600);                  
    
    //define LCD dimensions(16 columns, 2 rows)
    lcd.begin(16, 2); 
    lcd.print("Smart Clothesline");
    lcd.setCursor(0, 1);
    lcd.print("System Active...");
    delay(2000);
    lcd.clear();
}

void loop() {
    int sensorValue = analogRead(RAIN_SENSOR);
    Serial.println(sensorValue);
    
    if (sensorValue < RAIN_THRESHOLD) { 
        digitalWrite(BUZZER, HIGH);              //sounds the alarm
        winchServo.write(RETRACTED_POSITION);   //pull clothsline inside
    } else {
        digitalWrite(BUZZER, LOW);               //alarm off
        winchServo.write(EXTENDED_POSITION);    //clothesline outside
    }
    
    //refresh display screens every 1 second
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        
        // temperature voltage in celsious
        int rawTemp = analogRead(TEMP_SENSOR);
        float voltage = rawTemp * (5.0 / 1023.0);
        float temperatureC = (voltage - 0.5) * 100.0;
        float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;//convert to F
        
        bool alertStatus = (sensorValue < RAIN_THRESHOLD);
        updateDisplay(temperatureF, alertStatus);
    }
}

void updateDisplay(float temp, bool isRaining) {
    lcd.clear();
    lcd.setCursor(0, 0);
    
    if (isRaining) {
        lcd.print("Status: retracted");
        lcd.setCursor(0, 1);
        lcd.print("RAIN ALARM");
    } else {
        lcd.print("Status: dry");
        lcd.setCursor(0, 1);
        lcd.print("Temp: ");
        lcd.print(temp, 1);
        lcd.print("F");
    }
}