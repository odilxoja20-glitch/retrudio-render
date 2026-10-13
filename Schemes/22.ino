#include <LiquidCrystal.h>

// Pins for LEDs
const int GREEN_LED = 5;
const int YELLOW_LED = 4;
const int RED_LED = 3;

// Pins for Fans
const int MOTOR = 6;
const int TEMP_SENSOR = A0;

// Degree symbol
const char DEGREE = (char)178;

// Pins for LCD
const int RS = 8;
const int E = 9;
const int DB4 = 10;
const int DB5 = 11;
const int DB6 = 12;
const int DB7 = 13;

LiquidCrystal lcd(RS, E, DB4, DB5, DB6, DB7);

void setup()
{
    pinMode(GREEN_LED, OUTPUT);
    pinMode(YELLOW_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(MOTOR, OUTPUT);

    lcd.begin(16, 2);
}

void loop()
{
    int sensorValue = analogRead(TEMP_SENSOR);

    float voltage = sensorValue * (5.0 / 1023.0);
    float temperatureC = (voltage - 0.5) * 100.0;
    int temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;

    lcd.clear();
    lcd.setCursor(0, 0);

    lcd.print("Temp: ");
    lcd.print(temperatureF);
    lcd.print(DEGREE);
    lcd.print("F");

    if (temperatureF < 110)
    {
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(RED_LED, LOW);
        analogWrite(MOTOR, 0);

        lcd.setCursor(0, 1);
        lcd.print("Fan:  OFF ");
    }
    else if (temperatureF < 150)
    {
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(YELLOW_LED, HIGH);
        digitalWrite(RED_LED, LOW);
        analogWrite(MOTOR, 130);

        lcd.setCursor(0, 1);
        lcd.print("Fan:  LOW ");
    }
    else
    {
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(RED_LED, HIGH);
        analogWrite(MOTOR, 255);

        lcd.setCursor(0, 1);
        lcd.print("Fan:  HIGH");
    }

    delay(500);
}