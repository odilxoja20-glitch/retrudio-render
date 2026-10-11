#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int soilPin = A0;
const int buzzerPin = 8;
const int redLed = 5;
const int greenLed = 6;
const int blueLed = 7;

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  pinMode(buzzerPin, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int soilVal = analogRead(soilPin);
  int soilPercent = map(soilVal, 1023, 0, 0, 100);

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.print(F("Temp: ")); display.print(t); display.println(F(" C"));
  
  display.setCursor(0, 16);
  display.print(F("Air Hum: ")); display.print(h); display.println(F(" %"));
  
  display.setCursor(0, 32);
  display.print(F("Soil: ")); display.print(soilPercent); display.println(F(" %"));
  
  display.display();

  if (soilPercent < 30) {
    digitalWrite(redLed, HIGH);
    digitalWrite(greenLed, LOW);
    digitalWrite(blueLed, LOW);
    tone(buzzerPin, 1000);
  } else {
    digitalWrite(redLed, LOW);
    digitalWrite(greenLed, HIGH);
    digitalWrite(blueLed, LOW);
    noTone(buzzerPin);
  }

  delay(2000);
}