// Buzzer
//
// author:
//    Ben-An Chen @ kinetos.de
//
#include "pinout.h"

#include "Buzzer.h"

Buzzer::Buzzer(uint8_t pin) {
  this->pin = pin;

#ifdef ACTIVE_BUZZER
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
#endif // ACTIVE_BUZZER
}

#define BTIME 100
void Buzzer::beep(int times) {
  if (NOT_A_PIN == pin) return;
  times--;
  do {
#ifdef ACTIVE_BUZZER
    digitalWrite(pin, HIGH);
#else // PASSIVE_BUZZER
    tone(pin, 1000);
#endif // PASSIVE_BUZZER
    delay(BTIME);
#ifdef ACTIVE_BUZZER
    digitalWrite(pin, LOW);
#else // PASSIVE_BUZZER
    noTone(pin);
#endif // PASSIVE_BUZZER
    delay(BTIME);
  }
  while (times--);
}

Buzzer buzzer(BUZZER_PIN);