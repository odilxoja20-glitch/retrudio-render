const int trigPin = 8;
const int echoPin = 9;
const int ledPin = 7;
const int ld=6;
const int ldp=5;

long duration;
float distance;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ld, OUTPUT);
  pinMode(ldp, OUTPUT);
  Serial.begin(9600);
}

void loop() {

  // Send ultrasonic pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  // Measure echo time
  duration = pulseIn(echoPin, HIGH);

  // Calculate distance
  distance = duration * 0.0343 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Obstacle Detection
  if (distance > 0 && distance <= 200) {
    digitalWrite(ledPin, HIGH);
    Serial.println("Obstacle Detected!");
  } else {
    digitalWrite(ledPin, LOW);
    Serial.println("No Obstacle");
  }
  if (distance > 0 && distance <= 160) {
    digitalWrite(ld, HIGH);
    Serial.println("Obstacle Detected!");
  } else {
    digitalWrite(ld, LOW);
    Serial.println("No Obstacle");
  }
  if (distance > 0 && distance <= 100) {
    digitalWrite(ldp, HIGH);
    Serial.println("Obstacle Detected!");
  } else {
    digitalWrite(ldp, LOW);
    Serial.println("No Obstacle");
  }

  delay(200);
}