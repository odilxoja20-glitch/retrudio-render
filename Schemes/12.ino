#define BUZZER_PIN 8

// Ноты и их частоты
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_A4S 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587

// Мелодия Happy Birthday
int melody[] = {
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_F4, NOTE_E4,
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_G4, NOTE_F4,
  NOTE_C4, NOTE_C4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_E4, NOTE_D4,
  NOTE_A4S, NOTE_A4S, NOTE_A4, NOTE_F4, NOTE_G4, NOTE_F4
};

// Длительность нот (4 = четвертная нота, 8 = восьмая, 2 = половинная)
int noteDurations[] = {
  8, 8, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 2
};

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);

  int totalNotes = sizeof(melody) / sizeof(melody[0]);

  for (int thisNote = 0; thisNote < totalNotes; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(BUZZER_PIN, melody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN);
  }
}

void loop() {
  // Повтор воспроизведения раз в 5 секунд
  delay(5000);
  
  int totalNotes = sizeof(melody) / sizeof(melody[0]);
  for (int thisNote = 0; thisNote < totalNotes; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(BUZZER_PIN, melody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN);
  }
}