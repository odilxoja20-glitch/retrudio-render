const int soilSensorPin = A0;
const int relayPin = 7;
const int threshold = 400; // Porog vlajnosti dlya vklyucheniya nasosa

void setup() {
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // VyKlyucheno dlya rele s nizkim urovnem srabatyvaniya (Low-level trigger)
}

void loop() {
  int soilValue = analogRead(soilSensorPin);
  Serial.print("Vlajnost pochvy: ");
  Serial.println(soilValue);

  if (soilValue < threshold) {
    // Pochva suhaya, vklyuchaem nasos
    digitalWrite(relayPin, LOW);
    Serial.println("Poliv vklyuchen...");
    delay(5000); // Poliv 5 sekund
    digitalWrite(relayPin, HIGH);
    Serial.println("Poliv vyklyuchen.");
  }

  delay(10000); // Proverka kazhdye 10 sekund
}