#include <LiquidCrystal.h>
#include <Servo.h>

const int SENSOR = 8; //ULTRASONIC
const int PHOTORES = A0;   //PHOTORRESISTOR
const int NIGHT = 400;// MIDDLE OF PHOTORESISTOR LIMITS

int INVADERS = 0;

//lcd
const int RS = 2;
const int E = 3;
const int DB4 = 4;
const int DB5 = 5;
const int DB6 = 6;
const int DB7 = 7;
LiquidCrystal lcd(RS, E, DB4, DB5, DB6, DB7);
bool DETECTED = false; // no counted invader

//Servo motor SCARECROW
Servo SCARECROW;

//LED
const int LED1 = 11;
const int LED2 = 12;

//speaker
const int SPEAKER = 10;

void setup()
{
  Serial.begin(9600); //distance output monitor print
  //ULTRASONIC SENSOR
  pinMode(SENSOR, OUTPUT); 
  
  //LCD
  lcd.begin(16, 2); //setup lcd
  //lcd.print("I AM GROOT"); // test
  
  //SERVO MOTOR
  SCARECROW.attach(13);
  
  //LED
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  //SPEAKER
  pinMode(SPEAKER, OUTPUT);
  
}

void loop()
{
  long TIME = 0; // INITIALIZE DURATION
  float DIST = 0; // DISTANCE
  int BRIGHT = analogRead(PHOTORES);// INITIALIZE BRIGHTNESS VARIABLE
  
  //ULTRASONIC SENSOR
  pinMode(SENSOR, OUTPUT); //to trigger output of sensor
  digitalWrite(SENSOR, LOW); //sensor off
  delayMicroseconds(2);
  
  //Trigger pulse
  digitalWrite(SENSOR, HIGH); //sensor on
  delayMicroseconds(10); // on for 10micro secs
  digitalWrite(SENSOR, LOW); //turn off
  
  // read eccho
  pinMode(SENSOR, INPUT); //receives waves baack
  TIME = pulseIn(SENSOR, HIGH); //
  
  //CONVERT TO DISTANCE (cm)
  DIST = TIME *0.0343/2;
  
  //PRINT READINGS
  Serial.print("Distance: ");
  Serial.println(DIST);
  Serial.print("Light Level: ");
  Serial.println(BRIGHT);
  
  //when animal near 60cm turn on motor always, led when dark
  //sound and motor when day
  
  if (DIST <=60)
  {
    if (!DETECTED) //invader detected
    {
      INVADERS++;
      DETECTED = true; // keeps track and doesn't count same invader in the area
    }
    //scarecrow
    SCARECROW.write(0); //starting position
    delay(900);

    SCARECROW.write(180);// TURN 180 TURN BACK SECOND LOOP
    delay(900);
    
    if (BRIGHT < NIGHT) // WHEN DARK LED STROBE
    {
      //LED flashing loop
      for (int i = 0; i < 5; i++)
      {
        digitalWrite(LED1, HIGH); //led on
        digitalWrite(LED2, HIGH);
        delay(100);
        
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, LOW); //led off
        delay(100);
      }
    }
    else
      mariothemesong();
    
    //DISPLAY number of animals approached

    lcd.setCursor(0,0);
    lcd.print("I AM GROOT");

    lcd.setCursor(0,1);
    lcd.print("Saw ");
    lcd.print(INVADERS);
    lcd.print(" INVADERS!");
  }
  
  else //no animals detected
  { 
    DETECTED = false; //reset to count next invader
    SCARECROW.write(0);  //no action
    
    digitalWrite(LED1, LOW); // LED'S OFF
    digitalWrite(LED2, LOW);
    
    noTone(SPEAKER); // SPEAKER OFF
  }

}

void mariothemesong()
{
        tone(SPEAKER, 659.25, 167); // E5 bar1
		delay(167);
		tone(SPEAKER, 659.25, 167); // E5
		delay(334);
		tone(SPEAKER, 659.25, 167); // E5
		delay(334);
		tone(SPEAKER, 523.25, 167); // C5
		delay(167);
		tone(SPEAKER, 659.25, 334); // E5
		delay(334);
	
		tone(SPEAKER, 783.99, 334); // G5 bar2
		delay(668);
		tone(SPEAKER, 392.00, 334); // G4
		delay(668);

		tone(SPEAKER, 523.25, 334); // C5 bar3
		delay(501);
		tone(SPEAKER, 392.00, 334); // G4
		delay(501);
		tone(SPEAKER, 329.63, 334); // E4
		delay(501);

		tone(SPEAKER, 440.00, 334); // A4 bar4
		delay(334);
		tone(SPEAKER, 493.88, 334); // B4
		delay(334);
		tone(SPEAKER, 466.16, 167); // A#4
		delay(167);
		tone(SPEAKER, 440.00, 334); // A4
		delay(334);

		tone(SPEAKER, 392.00, 223); // G4 bar5
		delay(223);
		tone(SPEAKER, 659.25, 222); // E5
		delay(222);
		tone(SPEAKER, 783.99, 222); // G5
		delay(222);
		tone(SPEAKER, 880.00, 334); // A5
		delay(334);
		tone(SPEAKER, 698.46, 167); // F5
		delay(167);
		tone(SPEAKER, 783.99, 167); // G5
		delay(334);

		tone(SPEAKER, 659.25, 334); // E5 bar6
		delay(334);
		tone(SPEAKER, 523.25, 167); // C5
		delay(167);
		tone(SPEAKER, 587.33, 167); // D5
		delay(167);
		tone(SPEAKER, 493.88, 334); // B4
		delay(501);

		tone(SPEAKER, 523.25, 334); // C5 bar3-2
		delay(501);
		tone(SPEAKER, 392.00, 334); // G4
		delay(501);
		tone(SPEAKER, 329.63, 334); // E4
		delay(501);

		tone(SPEAKER, 440.00, 334); // A4 bar4-2
		delay(334);
		tone(SPEAKER, 493.88, 334); // B4
		delay(334);
		tone(SPEAKER, 466.16, 167); // A#4
		delay(167);
		tone(SPEAKER, 440.00, 334); // A4
		delay(334);

		tone(SPEAKER, 392.00, 223); // G4 bar5-2
		delay(223);
		tone(SPEAKER, 659.25, 222); // E5
		delay(222);
		tone(SPEAKER, 783.99, 222); // G5
		delay(222);
		tone(SPEAKER, 880.00, 334); // A5
		delay(334);
		tone(SPEAKER, 698.46, 167); // F5
		delay(167);
		tone(SPEAKER, 783.99, 167); // G5
		delay(334);
  
}