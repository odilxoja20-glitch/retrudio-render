#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Keypad.h>
#include <ESP32Servo.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

Servo myservo;
const int servoPin = 13;
const int buzzerPin = 14;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {23, 22, 1, 3}; 
byte colPins[COLS] = {21, 19, 18, 5}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);
  dht.begin();
  myservo.attach(servoPin);
  pinMode(buzzerPin, OUTPUT);

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

  char key = keypad.getKey();
  if (key) {
    Serial.println(key);
    if (key == '1') {
      myservo.write(90); // Otkrytiye zamka / dveri
      tone(buzzerPin, 1000, 200);
    } else if (key == '0') {
      myservo.write(0); // Zakrytiye zamka / dveri
      tone(buzzerPin, 500, 200);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("Temp: ")); display.print(t); display.println(F(" C"));
  display.setCursor(0, 16);
  display.print(F("Hum: ")); display.print(h); display.println(F(" %"));
  display.display();

  delay(2000);
}