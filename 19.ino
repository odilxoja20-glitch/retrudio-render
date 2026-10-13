/*16x2 LCD connection with Arduino Uno R3:
 # LCD RS -->D13
 # LCD  Enable(E)-->D12
 # LCD DB4-->D11
 # LCD DB5->D10
 # LCD DB6-->D9
 #  LCD DB7-->D8
 # LCD RW--> GND
 # LCD GND,V0,RW,LED(- CATHODE)-->pin to ground
  # LCD VCC,LED(+ ANODE)--> +5V
 
 HC-SR04 Ultrasonic Sensor connection With  Arduino Uno R3:
 #VCC--> +5V
 #TRIG--> D7
 #ECHO--> D6
 #GND--> pin  to ground
 */
int distance,time,inch;
#include<LiquidCrystal.h>
LiquidCrystal  lcd(13, 12, 11, 10, 9, 8);//LCD INTERFACE(RS,E,DB4,DB5,DB6 AND DB7 resprectively)

void  setup() {
pinMode(7,OUTPUT);  //Trig Pin
pinMode(6,INPUT);   //Echo Pin
Serial.begin(9600);
lcd.begin(16,2);
lcd.setCursor(0,0);
lcd.print("****WELCOME****");
delay(2000);
lcd.clear();  
lcd.setCursor(3,0);
lcd.print("DISTANCE");
lcd.setCursor(0,1);
lcd.print("MEASURING  DEVICE");
delay(2000);
lcd.clear(); 
lcd.setCursor(0,0);
lcd.print("  DEVICE STARTED ");
delay(2000);
lcd.clear(); 

}

void loop()  {
digitalWrite(7,LOW); //Trig Off
delayMicroseconds(20);
digitalWrite(7,HIGH);  //Trig ON
delayMicroseconds(20);
digitalWrite(7,LOW); //Trig Off
delayMicroseconds(20);
  
time = pulseIn(6, HIGH);//TO RECEIVE REFLECTED SIGNAL

distance= time*0.0340/2;
lcd.setCursor(0,1);  
lcd.print(distance);
lcd.print(" centimeter");

inch=distance*0.3937;//centimeter  to inch conversion
lcd.setCursor(0,0);
lcd.print(inch);
lcd.print(" inch");
  
Serial.println(time);  //To Print value on Serial Moniter 
Serial.println(inch);
Serial.println(distance);

delay(100);
}