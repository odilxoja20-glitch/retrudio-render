int ldrPin=A0;
int ldrread;
int dl=500;
int orange=3;
int green=4;
int buzzer=2;
void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
pinMode(ldrPin, INPUT);
pinMode(green, OUTPUT);
pinMode(orange, OUTPUT);
pinMode(buzzer, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:

ldrread=analogRead(ldrPin);
Serial.println(ldrread);
delay(dl);

if(ldrread>=60){
  digitalWrite(orange, HIGH);
  digitalWrite(green, LOW);
  digitalWrite(buzzer, LOW);
  
}

else{
  digitalWrite(green, HIGH);
    digitalWrite(orange, LOW);
     digitalWrite(buzzer, HIGH);
}

}