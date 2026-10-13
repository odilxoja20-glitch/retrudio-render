#include <LiquidCrystal.h>

// Pin defs
const int BUTTON = 7;   // Tickle button
const int GAME = 3;     // Minigame button
const int PR = A5;      // Photoresistor
const int PIEZO = 2;    // Buzzer
const int FSR_PIN = A0; // Force Sensor

// Distance sensor pins
const int TRIG_PIN = 5;
const int ECHO_PIN = 6;

// LCD Pins
const int RS = 8;
const int E = 9;
const int DB4 = 10;
const int DB5 = 11;
const int DB6 = 12;
const int DB7 = 13;

int prValue;
int buttonState;
int lastButtonState = LOW;
int gameState;
int buttonCount = 0;

// Timing vars
unsigned long lastSensorCheck = 0;
const int sensorInterval = 150; // Check sensors every 150 ms

// Melodies
int laughNotes[] = { 
  440, 523, 587, 659, 587, 523 
};
int laughDurations[] = { 
  100, 100, 100, 100, 100, 200 
};
int laughLength = 6;

int annoyedNotes[] = { 
  600, 520, 450, 400 
};
int annoyedDurations[] = { 
  180, 200, 230, 320 
};
int annoyedLength = 4;

int sleepNotes[] = { 
  523, 494, 440, 392, 330 
};
int sleepDurations[] = { 
  200, 220, 250, 300, 500 
};
int sleepLength = 5;

int singNotes[] = { 
  659, 784, 523, 587 
};
int singDurations[] = { 
  150, 150, 150, 300 
};
int singLength = 4;

int sleepSingNotes[] = { 
  440, 494, 523, 494, 440, 392, 440, 392 
};
int sleepSingDurations[] = { 
  180, 180, 220, 180, 180, 200, 180, 220 
};
int sleepSingLength = 8;

int awakeNotes[] = { 
  523, 659, 784, 988, 784, 659 
};
int awakeDurations[] = { 
  120, 120, 150, 170, 150, 200 
};
int awakeLength = 6;

int startGameNotes[] = { 
  659, 784, 988, 1319 
};
int startGameDurations[] = { 
  120, 120, 150, 200 
};
int startGameLength = 4;

// Game vars
int obsPos;        
bool jumping;
unsigned long jumpTimer = 0;
unsigned long jumpDuration = 500;
int score;

LiquidCrystal lcd(RS, E, DB4, DB5, DB6, DB7);

void setup() {
  Serial.begin(9600);
  
  pinMode(BUTTON, INPUT);
  pinMode(GAME, INPUT);
  pinMode(PR, INPUT);
  pinMode(FSR_PIN, INPUT);
  pinMode(PIEZO, OUTPUT);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lcd.begin(16, 2);
}

void loop() {
  prValue = analogRead(PR);

  if (prValue < 932) {
    while (prValue < 932) {
      sleep();
    }
  } else {
    // Waking up animation
    playMelody(awakeNotes, awakeDurations, awakeLength);
    lcd.setCursor(6, 0);
    lcd.print("- -       ");
    lcd.setCursor(7, 1);
    lcd.print("O         ");
    delay(1000);

    lcd.setCursor(6, 0);
    lcd.print("^ ^       ");
    lcd.setCursor(7, 1);
    lcd.print("-         ");
    delay(1000);

    resetFace();
    delay(1000);

    // Main awake loop
    while (prValue >= 932) {
      awake();
    }

    // Faling asleep animation
    playMelody(sleepNotes, sleepDurations, sleepLength);
    lcd.setCursor(6, 0);
    lcd.print("- -       ");
    lcd.setCursor(6, 1);
    lcd.print("\\_/       ");
    delay(1000);

    lcd.setCursor(6, 0);
    lcd.print("- -       ");
    lcd.setCursor(6, 1);
    lcd.print(" O        ");
    delay(1000);

    lcd.setCursor(6, 0);
    lcd.print("- -       ");
    lcd.setCursor(6, 1);
    lcd.print(" o        ");
    delay(1000);
  }
}

void awake() {
  buttonState = digitalRead(BUTTON);

  // Tickle button logic
  if (buttonState == HIGH && lastButtonState == LOW) {
    buttonCount++; 

    if (buttonCount <= 3) {
      lcd.setCursor(6, 0);
      lcd.print("> < HAHA! ");
      lcd.setCursor(6, 1);
      lcd.print("\\_/       ");
      playMelody(laughNotes, laughDurations, laughLength);
      resetFace();
    } else {
      lcd.setCursor(6, 0);
      lcd.print("> < STOP! ");
      lcd.setCursor(6, 1);
      lcd.print(" ^        ");
      playMelody(annoyedNotes, annoyedDurations, annoyedLength);
      delay(500);
      resetFace();
    }
  }

  lastButtonState = buttonState;

  // Sensor checking loop (distance & force)
  if (millis() - lastSensorCheck > sensorInterval) {
    lastSensorCheck = millis();
    
    int forceValue = analogRead(FSR_PIN);
    long distance = getDistance();

    // Force sensor priority
    if (forceValue > 600) {
      // Hard squeeze / squish
      lcd.setCursor(6, 0);
      lcd.print("X X  Owie!");
      lcd.setCursor(6, 1);
      lcd.print(" o        ");
      tone(PIEZO, 150, 200); // Low groan tone
      delay(300);
      resetFace();
    } 
    else if (forceValue > 100) {
      // Gentle pet / soft touch
      lcd.setCursor(6, 0);
      lcd.print("<3 <3  Aw!");
      lcd.setCursor(6, 1);
      lcd.print("  u        ");
      tone(PIEZO, 1046, 150); // Happy chirp
      delay(300);
      resetFace();
    } 
    // Distance sensor
    else if (distance > 0 && distance < 60) {
      // Too close, invading personal space
      lcd.setCursor(6, 0);
      lcd.print("O O  Whoa!");
      lcd.setCursor(6, 1);
      lcd.print(" o        ");
      tone(PIEZO, 988, 100);
    } 
    else if (distance >= 60 && distance < 80) {
      // Noticing user getting close
      lcd.setCursor(6, 0);
      lcd.print("o o  Hmm? ");
      lcd.setCursor(6, 1);
      lcd.print(" -        ");
    } 
    else {
      // Default happy state
      resetFace();
    }
  }

  // Minigame button
  gameState = digitalRead(GAME);
  if (gameState == HIGH) {
    runGame();
  }

  // Random singing
  int chance = random(1, 100); 
  if (chance == 10) {
    lcd.setCursor(6, 0);
    lcd.print("^ ^       ");
    lcd.setCursor(6, 1);
    lcd.print(" O   LA LA");
    playMelody(singNotes, singDurations, singLength);
    resetFace();
  }

  prValue = analogRead(PR);
  delay(30); 
}

// Helper function - read distance (cm)
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000); 
  if (duration == 0) return 100; 
  return duration * 0.034 / 2;
}

// Helper function - cleanly reset face & wipe all trailing chars
void resetFace() {
  lcd.setCursor(6, 0);
  lcd.print("^ ^       "); 
  lcd.setCursor(6, 1);
  lcd.print("\\_/       ");
}

void sleep() {
  lcd.setCursor(6, 0);
  lcd.print("- -       ");
  lcd.setCursor(0, 1);
  lcd.print("                "); 
  lcd.setCursor(7, 1);
  lcd.print("-    ");
  delay(1000);

  lcd.setCursor(6, 0);
  lcd.print("- -       ");
  lcd.setCursor(7, 1);
  lcd.print("o zZ ");

  int chance = random(1, 50); 
  if (chance == 10) {
    playMelody(sleepSingNotes, sleepSingDurations, sleepSingLength);
  }

  delay(1000);
  prValue = analogRead(PR);
}

void playMelody(int notes[], int durations[], int length) {
  for (int i = 0; i < length; i++) {
    tone(PIEZO, notes[i], durations[i]);
    delay(durations[i] + 20);
  }
  noTone(PIEZO);
}

void runGame() {
  lcd.clear();

  obsPos = 15;
  jumping = false;
  score = 0;

  delay(500);
  playMelody(startGameNotes, startGameDurations, startGameLength);

  while (true) {
    lcd.clear();

    if (digitalRead(GAME) == HIGH && !jumping) {
      jumping = true;
      jumpTimer = millis();
    }

    if (jumping && millis() - jumpTimer > jumpDuration) {
      jumping = false;
    }

    if (jumping) {
      lcd.setCursor(2, 0); 
      lcd.print("O");
    } else {
      lcd.setCursor(2, 1);
      lcd.print("O");
    }

    if (obsPos >= 0) {
      lcd.setCursor(obsPos, 1);
      lcd.print("/\\");
    }

    if (!jumping && obsPos == 2) {
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("GAME OVER");
      lcd.setCursor(4, 1);
      lcd.print("Score: ");
      lcd.print(score);
      playMelody(sleepNotes, sleepDurations, sleepLength);
      delay(2500);
      
      lcd.clear(); 
      resetFace();
      return;
    }

    obsPos--;

    if (obsPos < -1) {
      obsPos = 15;
      score++;
    }

    delay(120);
  }
}