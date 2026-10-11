#include <Servo.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

// declare constants for lcd
const int RS = 2;
const int E = 3;
const int DB4 = 4;
const int DB5 = 5;
const int DB6 = 6;
const int DB7 = 7;

// constants needed for servo 
// Declare SERVO pin 
const int SERVO_PIN = A0;

// constants for motion sensor and RGB LED
const int PIR = A5;
const int ALARM = A4;
const int RED_LED = A3;
const int BLUE_LED = A2;
const int GREEN_LED = A1;

/*********************** Piezo Speaker/Alarm ***********************/
// different states the security system can be in
const int UNARMED = 0;
const int ARMED = 1;
const int ALARM_STATUS = 2;

// stores current state of security system
int systemState = UNARMED;

/*********************** MOTION SENSOR ***********************/
// keep track of PIR motion sensor
int pirState;

/*********************** LCD ***********************/
// instantiate LCD object
LiquidCrystal lcd(RS, E, DB4, DB5, DB6, DB7);

/*********************** Servo Motor ***********************/
// servo in locked and unlocked positions
const int LOCKED_POS = 0;
const int UNLOCKED_POS = 90;

// create servo object
Servo servo;

/*********************** Keypad ***********************/
// Constants for keypad
// only using digits 1 - 9
// only need col 1 - 3 and rows 1 - 3
const byte ROWS = 3;
const byte COLS = 3;

// keymap defines keys pressed according row & col
const char KEYMAP[ROWS][COLS]=
{
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
};

// code for Keypad
const String CODE = "1235";

// code that shows keypad connections to arduino pins
byte ROW_PINS[ROWS] = {13, 12, 11}; // Rows 0 - 2 on keypad
byte COL_PINS[COLS] = {10, 9, 8}; // columns 0 - 2 on keypad


// instantiate keypad object
Keypad keypad = Keypad(makeKeymap(KEYMAP), ROW_PINS, COL_PINS, ROWS, COLS);

// stores keypad input from user
String input = "";

// stores the current key pressed
char key;

// counts number of incorrect code attempts
int failedAttempts = 0;

// keeps track of alarm disarm screen
bool disarmScreen = false;


void setup(){
  // establish size of lcd screen
  lcd.begin(16, 2);
  
  // attach servo to pin
  servo.attach(SERVO_PIN);
  
  // set sensor pins as inputs and output devices as outputs
  pinMode(PIR, INPUT);
  pinMode(ALARM, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  // update all outputs to match starting state
  updateOutputs();
}

void loop(){
  // check if user entered into keypad 
  readKeypad();
  
  // check if motion is detected
  readPIR();
  
  // update outputs based on system status
  updateOutputs();
}

/*********************** FUNCTIONS ***********************/

// readKeypad: reads keypad input and controls the
// arming/disarming and activates the alarm
void readKeypad(){
  // read key from keypad
  key = keypad.getKey();

  // check to see if there is a key (user typed something)
  // is not null
  if (key != NO_KEY) {

    // show disarm screen when alarm is active
    if (systemState == ALARM_STATUS)
    {
        disarmScreen = true;
    }

    input += key;
    
    // check to see if input is 4 digits long
    if (input.length() == 4){
      
      // check to see if code is correct (1235)
      if (input == CODE) {

        // reset failed attempts after successful code entered
        failedAttempts = 0;

        lcd.setCursor(6, 0);
        lcd.print("CORRECT");
        delay(500);

        // to turn alarm off and disarm system
        if(systemState == ALARM_STATUS){
          alarmOff();
          systemState = UNARMED;
          disarmScreen = false;
        }

        // arm system if currently unarmed
        else if(systemState == UNARMED){
          systemState = ARMED;
        }

        // unarm system if currently armed
        else{
          systemState = UNARMED;
        }
        
      }
      else {

        // increase failed attempt counter
        failedAttempts++;
        
        lcd.setCursor(6, 0);
        lcd.print("WRONG");
        
        delay(500);
       
        // activate alarm after 3 incorrect tries
        if(failedAttempts >= 3){
          systemState = ALARM_STATUS;
          disarmScreen = false;
          alarmOn();
        }
      }
      
      // clear keypad input after checking code
      input = "";
    }
  }  
}


// readPIR: detects motion from PIR sensor and sets
// off alarm when the system is armed
void readPIR() {

  // read motion sensor value
  pirState = digitalRead(PIR);

  // activate alarm if motion is detected while armed
  if(systemState == ARMED && pirState == HIGH)
  {
    systemState = ALARM_STATUS;
    disarmScreen = false;
    alarmOn();
  }
}

// updateOutputs: updated the LCD, servo, LED and alarm 
// based on if system is armed or disarmed
void updateOutputs(){

  // update lcd status display
  // unarmed vs armed
  showStatus();
  
  // change outputs depending on system state
  if(systemState == UNARMED){

    // unlock door and display safe color
    unlockDoor();
    displayColor(0, 255, 0); // green light
    alarmOff();

  }
  else if(systemState == ARMED){

    // lock door and display armed color
    lockDoor();
    displayColor(255, 0, 0); // red light
    alarmOff();

  }
  else if(systemState == ALARM_STATUS){

    // lock door and activate alarm color
    lockDoor();
    // combine red and white to simulate flashing light
    displayColor(255, 0, 0);   // red light
    delay(50);

    displayColor(255, 255, 255); // white light
    delay(50); 
    alarmOn();
  }
}

// displayColor: controls RGB LED brightness
// red, green, blue
void displayColor(int r, int g, int b){

  // control brightness of each RGB LED color
  analogWrite(RED_LED, r);
  analogWrite(GREEN_LED, g);
  analogWrite(BLUE_LED, b);
}


// lockDoor: rotates servo to lock 
// lock -> 0 degrees
void lockDoor(){

  // move servo to locked position
  servo.write(LOCKED_POS);
}

// unlockDoor: rotates servo to unlock
// unlock -> 90 degrees
void unlockDoor(){

  // move servo to unlocked position
  servo.write(UNLOCKED_POS);
}

// showStatus: displays keypad input and status onto LCD
void showStatus(){

  // display keypad input when alarm is not active
  if(systemState != ALARM_STATUS)
    {
      lcd.setCursor(0,0);
      lcd.print("CODE: ");

      // clear old input before displaying new input
      lcd.setCursor(6,0);
      lcd.print("          ");
      lcd.setCursor(6,0);
      lcd.print(input);

      lcd.setCursor(0,1);
  }

  // display message based on current system state
  switch(systemState)
  {
      case UNARMED:
        lcd.print("Status: UNARMED");
        break;

      case ARMED:
        lcd.print("Status: ARMED  ");
        break;

      case ALARM_STATUS:

        // display security warning until user enters code
        if (!disarmScreen)
        {
          lcd.setCursor(0,0);
          lcd.print("CALLING SECURITY");
          lcd.setCursor(0,1);
          lcd.print(" ***INTRUDER*** ");
        }

        // allow user to disarm alarm
        else{
          lcd.setCursor(0,0);
          lcd.print("Enter Code To   ");

          lcd.setCursor(0,1);
          lcd.print("Disarm: ");
          lcd.print(input);
          lcd.print("        "); 
        }
    
        break;
  }

  // move cursor back to input location
  lcd.setCursor(6,0);  
}

// alarmOn: turns on alarm speaker
void alarmOn(){

  // activate speaker alarm
  tone(ALARM, 500);
}

//// alarmOn: turns off alarm speaker
void alarmOff(){

  // turn off speaker alarm
  noTone(ALARM);
}