// Buzzer
//
// author:
//    Ben-An Chen @ kinetos.de
//
#ifndef __BUZZER_H__
#define __BUZZER_H__

#include <Arduino.h>

typedef enum {
  BEEP_START              =  1,
  BEEP_DONE_OK,           // 2
  BEEP_DONE_NOK,          // 3
  BEEP_ERR_ENTER_PMODE,   // 4
  BEEP_ERR_OPEN_FILE,     // 5
  BEEP_ERR_PROG_PAGE,     // 6
  BEEP_ERR_READ_PAGE,     // 7
  BEEP_ERR_LEAVE_PMODE    // 8
} Beep_t;

class Buzzer
{
private:
  uint8_t pin;

public:
  Buzzer(uint8_t pin);

  void beep(int times);
};

extern Buzzer buzzer;

#endif // __BUZZER_H__