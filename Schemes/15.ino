const int relayPin = 7;

void setup() {
  pinMode(relayPin, OUTPUT);
}

void loop() {
  digitalWrite(relayPin, HIGH); // Vklyuchit rele (i nagruzku)
  delay(3000);                  // Zhdat 3 sekundy
  digitalWrite(relayPin, LOW);  // Vyklyuchit rele (i nagruzku)
  delay(3000);                  // Zhdat 3 sekundy
}