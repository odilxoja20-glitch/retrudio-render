#include <LiquidCrystal.h>

/*
  Arduino Smart Piano with Light-Controlled Sustain

  Project purpose:
  This project works like a small electronic piano.
  The user presses push buttons to play different notes.
  The LCD shows the note, octave, light mode, and sensor information.
  The potentiometer changes the octave.
  The photoresistor controls sustain mode.

  LCD pins:
  RS -> 8
  E  -> 9
  D4 -> 10
  D5 -> 11
  D6 -> 12
  D7 -> 13

  Button note mapping:
  C      -> A1
  D      -> A2
  E      -> 2
  F      -> 3
  G      -> 4
  A      -> 5
  B      -> 6
  High C -> 7

  Buzzer:
  Pin A3

  Potentiometer:
  Pin A0

  Photoresistor:
  Pin A5

  Button behavior with pull-down resistors:
  Not pressed = LOW
  Pressed     = HIGH

  Photoresistor feature:
  Bright light = sustain mode
  Dark / covered photoresistor = normal mode
*/

// LCD object setup
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

// Pin constants
const int buzzerPin = A3;
const int octavePotPin = A0;
const int lightSensorPin = A5;

// Button pins
const int buttonPins[8] = {
  A1, A2, 2, 3, 4, 5, 6, 7
};

// Note names
const String noteNames[8] = {
  "C", "D", "E", "F", "G", "A", "B", "C5"
};

// Base note frequencies in Hertz
const int baseFrequencies[8] = {
  262, 294, 330, 349, 392, 440, 494, 523
};

// Button debounce variables
const unsigned long debounceDelay = 25;
int lastButtonReading[8];
int stableButtonState[8];
unsigned long lastDebounceTime[8];

// Photoresistor / sustain variables
const int brightThreshold = 600;
const unsigned long sustainTime = 800;

int lastFrequency = 0;
String lastNoteName = "";
bool sustaining = false;
unsigned long sustainStartTime = 0;

// State tracking variables
int currentButton = -1;
int previousButton = -99;
int currentFrequency = 0;
int previousFrequency = -1;

// LCD update variables
unsigned long lastIdleUpdate = 0;
const unsigned long idleUpdateInterval = 700;

String lastLine1 = "";
String lastLine2 = "";

void setup() {
  // Start the LCD with 16 columns and 2 rows.
  lcd.begin(16, 2);

  // Set up the buzzer.
  pinMode(buzzerPin, OUTPUT);
  noTone(buzzerPin);

  // Set up all piano buttons as inputs.
  for (int i = 0; i < 8; i++) {
    pinMode(buttonPins[i], INPUT);

    lastButtonReading[i] = digitalRead(buttonPins[i]);
    stableButtonState[i] = lastButtonReading[i];
    lastDebounceTime[i] = 0;
  }

  // Startup message
  showLCD("Arduino Piano", "Smooth Mode");
  delay(1200);

  // Short startup sound to confirm the buzzer works.
  tone(buzzerPin, 440);
  delay(120);
  noTone(buzzerPin);
  delay(80);
  tone(buzzerPin, 523);
  delay(120);
  noTone(buzzerPin);

  showLCD("Ready to play", "Press a key");
}

void loop() {
  // Read analog sensors.
  int octaveValue = analogRead(octavePotPin);
  int lightValue = analogRead(lightSensorPin);

  // Convert potentiometer value into octave setting.
  int octaveShift = getOctaveShift(octaveValue);

  // Bright light activates sustain mode.
  bool sustainMode = lightValue > brightThreshold;

  // Check whether a piano key is pressed.
  currentButton = readPressedButton();

  if (currentButton != -1) {
    handleButtonPressed(currentButton, octaveShift, lightValue, sustainMode);
  } else {
    handleNoButton(octaveValue, lightValue, octaveShift, sustainMode);
  }
}

void handleButtonPressed(int buttonIndex, int octaveShift, int lightValue, bool sustainMode) {
  // Find the frequency for the selected note and octave.
  currentFrequency = calculateFrequency(baseFrequencies[buttonIndex], octaveShift);

  // Only restart the tone if the frequency changed.
  if (currentFrequency != previousFrequency) {
    tone(buzzerPin, currentFrequency);
    previousFrequency = currentFrequency;
  }

  // Save this note in case sustain mode is used after release.
  lastFrequency = currentFrequency;
  lastNoteName = noteNames[buttonIndex];
  sustaining = false;

  // Update the LCD only when the selected button changes.
  if (buttonIndex != previousButton) {
    String line1 = "Note:" + noteNames[buttonIndex] + " " + String(currentFrequency) + "Hz";

    String line2;

    if (sustainMode) {
      line2 = "Sustain ";
    } else {
      line2 = "Normal  ";
    }

    line2 += octaveName(octaveShift);
    line2 += " L:";
    line2 += String(lightValue / 10);

    showLCD(line1, line2);

    previousButton = buttonIndex;
  }
}

void handleNoButton(int octaveValue, int lightValue, int octaveShift, bool sustainMode) {
  previousButton = -1;
  previousFrequency = -1;

  // If sustain mode is active, continue the last note briefly after release.
  if (sustainMode && lastFrequency > 0) {
    if (!sustaining) {
      sustaining = true;
      sustainStartTime = millis();
      tone(buzzerPin, lastFrequency);
    }

    if (millis() - sustainStartTime < sustainTime) {
      String line1 = "Sustain:" + lastNoteName;
      String line2 = "Light:" + String(lightValue);
      showLCD(line1, line2);
      return;
    }
  }

  // Stop sound when no key is pressed and sustain is not active.
  noTone(buzzerPin);

  lastFrequency = 0;
  sustaining = false;

  // Update idle screen slowly to reduce flickering.
  if (millis() - lastIdleUpdate >= idleUpdateInterval) {
    lastIdleUpdate = millis();

    String line1;

    if (sustainMode) {
      line1 = "Mode:Sustain";
    } else {
      line1 = "Mode:Normal";
    }

    String line2 = "L:" + String(lightValue) + " Oct:" + octaveName(octaveShift);

    showLCD(line1, line2);
  }
}

int readPressedButton() {
  /*
    Returns:
    0 to 7 = button index
    -1     = no button pressed
  */

  for (int i = 0; i < 8; i++) {
    int reading = digitalRead(buttonPins[i]);

    // Reset debounce timer when a button reading changes.
    if (reading != lastButtonReading[i]) {
      lastDebounceTime[i] = millis();
      lastButtonReading[i] = reading;
    }

    // Accept the reading after it has been stable.
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      stableButtonState[i] = reading;
    }

    // With pull-down resistors, HIGH means pressed.
    if (stableButtonState[i] == HIGH) {
      return i;
    }
  }

  return -1;
}

int getOctaveShift(int potValue) {
  /*
    Potentiometer range:
    0-340    = low octave
    341-681  = middle octave
    682-1023 = high octave
  */

  if (potValue < 341) {
    return -1;
  } else if (potValue < 682) {
    return 0;
  } else {
    return 1;
  }
}

int calculateFrequency(int baseFrequency, int octaveShift) {
  // Octaves are created by halving or doubling the base frequency.

  if (octaveShift == -1) {
    return baseFrequency / 2;
  } else if (octaveShift == 1) {
    return baseFrequency * 2;
  } else {
    return baseFrequency;
  }
}

String octaveName(int octaveShift) {
  if (octaveShift == -1) {
    return "Low";
  } else if (octaveShift == 1) {
    return "High";
  } else {
    return "Mid";
  }
}

void showLCD(String line1, String line2) {
  /*
    Updates the LCD smoothly by only changing text
    when the message is different.
  */

  line1 = padTo16(line1);
  line2 = padTo16(line2);

  if (line1 != lastLine1) {
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lastLine1 = line1;
  }

  if (line2 != lastLine2) {
    lcd.setCursor(0, 1);
    lcd.print(line2);
    lastLine2 = line2;
  }
}

String padTo16(String text) {
  /*
    Pads text with spaces so old characters do not remain
    on the LCD when a shorter message is printed.
  */

  if (text.length() > 16) {
    text = text.substring(0, 16);
  }

  while (text.length() < 16) {
    text += " ";
  }

  return text;
}