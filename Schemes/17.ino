#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// OLED Configuration (I2C: SDA=A4, SCL=A5)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pin Definitions
#define SOIL_MOISTURE_PIN A0   // Soil Moisture Sensor
#define DHT_PIN 2              // DHT11 Sensor
#define DHT_TYPE DHT11         // DHT11 Sensor Type
#define BUZZER_PIN 6           // Buzzer

// Tri-Color LED (Common Anode)
#define RED_LED 3              // CO1 - Red
#define GREEN_LED 4            // CO2 - Green
#define BLUE_LED 5             // CO3 - Blue

// Initialize DHT Sensor
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(SOIL_MOISTURE_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);

    // Initialize OLED Display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED Display not found!");
        while (1);
    }
    display.clearDisplay();
    Serial.println("System Initialized!");
}

// Function to set LED color
void setColor(bool red, bool green, bool blue) {
    digitalWrite(RED_LED, red ? LOW : HIGH);
    digitalWrite(GREEN_LED, green ? LOW : HIGH);
    digitalWrite(BLUE_LED, blue ? LOW : HIGH);
}

void loop() {
    // Read Raw Soil Moisture Value
    int moistureValue = analogRead(SOIL_MOISTURE_PIN);
    Serial.print("Raw Moisture Value: ");
    Serial.println(moistureValue);

    // Adjust Mapping Based on Real Readings
    int moisturePercentage = map(moistureValue, 300, 900, 0, 100);
    moisturePercentage = constrain(moisturePercentage, 0, 100);

    // Read Temperature & Humidity
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Check if DHT11 gives valid data
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("DHT11 Sensor Not Found!");
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(10, 20);
        display.print("DHT11 Not Found!");
        display.display();
        delay(2000);
        return;
    }

    // Print values in Serial Monitor
    Serial.print("Moisture: "); Serial.print(moisturePercentage); Serial.println("%");
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");

    // Display Sensor Data on OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(10, 10);
    display.print("Moisture: "); display.print(moisturePercentage); display.println("%");
    display.setCursor(10, 30);
    display.print("Temp: "); display.print(temperature); display.println("C");
    display.setCursor(10, 50);
    display.print("Humidity: "); display.print(humidity); display.println("%");
    display.display();

    // LED & Buzzer Alerts
    if (moisturePercentage < 30 || temperature > 30.0 || temperature < 20.0) {
        setColor(1, 0, 0);  // 🔴 Red (Critical Condition)
        digitalWrite(BUZZER_PIN, HIGH);
    } else if (moisturePercentage < 40) {
        setColor(0, 0, 1);  // 🔵 Blue (Warning Condition)
        digitalWrite(BUZZER_PIN, LOW);
    } else {
        setColor(0, 1, 0);  // 🟢 Green (Normal Condition)
        digitalWrite(BUZZER_PIN, LOW);
    }

    delay(2000);  // Refresh every 2 seconds
}