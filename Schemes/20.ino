//Libraries for the LCD, and NeoPixel
#include <LiquidCrystal.h>
#include <Adafruit_NeoPixel.h>

//PIN Constants
//LED pins
const int BLUE_LED = A0;
const int GREEN_LED = A1;
const int RED_LED = A2;

//Piezo pin
const int ALARM = 2;

//Movement buttons
const int USER1_UP_BUTTON = 3;
const int USER1_DOWN_BUTTON = 4;

const int USER2_UP_BUTTON = 5;
const int USER2_DOWN_BUTTON = 6;

//LCD pins
const int RS = 7;
const int E = 8;
const int DB4 = 9;
const int DB5 = 10;
const int DB6 = 11;
const int DB7 = 12;

//NeoPixel pin
const int NEOPIXEL = 13;

const int ROWS = 10;
const int COLUMNS = 10;
const int NUM_PIXELS = 100;

//2d matrix to represent the playing board
int matrix[10][10] = {
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0}
};

//Initializing the LCD and NeoPixel
LiquidCrystal lcd(RS, E, DB4, DB5, DB6, DB7);
//initializes with the number of pixels in the chain, the pin the input is coming from, and the color info
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL, NEO_GRB + NEO_KHZ800);

//Variables for the bumpers and ball
int bumper1X = 0;
int bumper1Y1 = 4; //user 1 bumper starting location
int bumper1Y2 = 5;

int bumper2X = 9;
int bumper2Y1 = 4; //user 2 bumper starting location
int bumper2Y2 = 5;

int ballX = 5; //ball starting location
int ballY = 5;

int moveX = 1; //variables for increments of movement for both the bumpers and ball
int moveY = 1;

bool isActiveGame = true;
int activeGameBuffer = 0;

int pixelCounter = 0;

void setup()
{
  //Initializing pins
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  
  pinMode(ALARM, OUTPUT);
  
  pinMode(USER1_UP_BUTTON, INPUT);
  pinMode(USER1_DOWN_BUTTON, INPUT);
  
  pinMode(USER2_UP_BUTTON, INPUT);
  pinMode(USER2_DOWN_BUTTON, INPUT);
  
  //Initializing LDC, NeoPixel and 7-segment
  //LCD
  lcd.begin(16, 2);
  //NeoPixel
  pixels.begin();
  pixels.setBrightness(255);
  
  //Initializing board
  //bumper 1
  matrix[bumper1Y1][bumper1X] = 1;
  matrix[bumper1Y2][bumper1X] = 1;
  //bumper 2
  matrix[bumper2Y1][bumper2X] = 2;
  matrix[bumper2Y2][bumper2X] = 2;
  //ball
  matrix[ballY][ballX] = 3;
  
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      if (matrix[i][j] == 1) {
        pixels.setPixelColor(pixelCounter, pixels.Color(255, 0, 0));
      } else if (matrix[i][j] == 2) {
        pixels.setPixelColor(pixelCounter, pixels.Color(0, 0, 255));
      } else if (matrix[i][j] == 3) {
        pixels.setPixelColor(pixelCounter, pixels.Color(0, 255, 0));
      } else {
        pixels.setPixelColor(pixelCounter, pixels.Color(0, 0, 0));
      }
      pixelCounter++;
    }
  }
  pixels.show();
  
}

void loop()
{
  pixelCounter = 0;
  
  //Input from the buttons to move the bumpers
  //Pull DOWN resistors on the breadboard
  //RED UP AND DOWN
  if (digitalRead(USER1_UP_BUTTON)) {
    if (bumper1Y1 <= 9 && bumper1Y2 <= 8) {
     
      //updating matrix for ball comparisons to bounce
      matrix[bumper1Y1][bumper1X] = 0;
      matrix[bumper1Y2][bumper1X] = 0;
    
      bumper1Y1 += 1;
      bumper1Y2 += 1;
      
      matrix[bumper1Y1][bumper1X] = 1;
      matrix[bumper1Y2][bumper1X] = 1;
    }
  }
  if (digitalRead(USER1_DOWN_BUTTON)) {
    if (bumper1Y1 > 0 && bumper1Y2 > 1) {
    
      matrix[bumper1Y1][bumper1X] = 0;
      matrix[bumper1Y2][bumper1X] = 0;
      
      bumper1Y1 -= 1;
      bumper1Y2 -= 1;
      
      matrix[bumper1Y1][bumper1X] = 1;
      matrix[bumper1Y2][bumper1X] = 1;
    }
  }
  
  //BLUE UP AND DOWN
  if (digitalRead(USER2_UP_BUTTON)) {
    if (bumper2Y1 <= 9 && bumper2Y2 <= 8) {
    
      matrix[bumper2Y1][bumper2X] = 0;
      matrix[bumper2Y2][bumper2X] = 0;
      
      bumper2Y1 += 1;
      bumper2Y2 += 1;
      
      matrix[bumper2Y1][bumper2X] = 2;
      matrix[bumper2Y2][bumper2X] = 2;
    }
  }
  if (digitalRead(USER2_DOWN_BUTTON)) {
    if (bumper2Y1 > 0 && bumper2Y2 > 1) {
    
      matrix[bumper2Y1][bumper2X] = 0;
      matrix[bumper2Y2][bumper2X] = 0;
      
      bumper2Y1 -= 1;
      bumper2Y2 -= 1;
      
      matrix[bumper2Y1][bumper2X] = 2;
      matrix[bumper2Y2][bumper2X] = 2;
    }
  }
  
  //Moving the ball
  int tempX = ballX + moveX;
  int tempY = ballY + moveY;
  
  if(!isActiveGame) { //gives time by allowing loops to occur
    activeGameBuffer++; //still allows bumpers to be moved
  }
  if (activeGameBuffer >= 10) {
    activeGameBuffer = 0;
    lcd.clear();
    displayColor(0,0, 0);
    isActiveGame = true;
  } //restarts the game by setting it to true.
  
  //If the ball hits the top or bottom wall
  if (tempY <= 0 || tempY >= 9) {
    moveY = -moveY; //changes the direction of the ball
  }
  
  //If the ball hits the bumpers
  if (tempX == 0 || tempX == 9) {
    if (matrix[tempY][0] == 1 || matrix[tempY][9] == 2) {
      moveX = -moveX;
      tempX = ballX + moveX;
    } else {
      //reset the ball's position 
      //display win message on lcd, play sound, and update LED to who won
      if (tempX == 0) {
        lcd.setCursor(0, 0);
        lcd.print("BLUE SCORES!");
        tone(ALARM, 200, 250);
        displayColor(0, 0, 255);
      } else if (tempX == 9) {
        lcd.setCursor(0, 0);
        lcd.print("RED SCORES! ");
        tone(ALARM, 500, 250);
        displayColor(255, 0, 0);
      }
      lcd.setCursor(0, 1);
      lcd.print("Restarting...");
      
      //then restart the game
      matrix[ballY][ballX] = 0;
      ballX = 5;
      ballY = 5;
      moveX = 1;
      moveY = 1;
      matrix[ballY][ballX] = 3;
      
      //delay ~2 seconds to allow players to be ready for the next game
      isActiveGame = false;
    }
  }
  
  if (isActiveGame) { //controls movement of the ball while the game is active
    matrix[ballY][ballX] = 0;
    
    ballX = tempX;
    ballY = tempY;
    
    matrix[ballY][ballX] = 3;
  }
 
  //updates the pixels every loop to display accurate information
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      if (matrix[i][j] == 1) {
        pixels.setPixelColor(pixelCounter, pixels.Color(255, 0, 0));
      } else if (matrix[i][j] == 2) {
        pixels.setPixelColor(pixelCounter, pixels.Color(0, 0, 255));
      } else if (matrix[i][j] == 3) {
        pixels.setPixelColor(pixelCounter, pixels.Color(0, 255, 0));
      } else {
        pixels.setPixelColor(pixelCounter, pixels.Color(0, 0, 0));
      }
      pixelCounter++;
    }
  }
  pixels.show();
  
  delay(250);
}

//helper method for coloring the RGB LED
void displayColor(int r, int g, int b) {
  analogWrite(RED_LED, r);
  analogWrite(GREEN_LED, g);
  analogWrite(BLUE_LED, b);
}