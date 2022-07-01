// LCD1602 I2C
//
// author:
//    Ben-An Chen @ kinetos.de
//
#ifndef _LCD1602_H_
#define _LCD1602_H_

#include <Wire.h>
#include "./LiquidCrystal_I2C.h"

#define LCD_MAX_CHARS_PER_LINE  16

class LCD1602
{
private:
  LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
  char strBuf[LCD_MAX_CHARS_PER_LINE + 1];

public:
  LCD1602() { }

  inline void begin(int x, int y) { Wire.begin(); lcd.begin(x,y); }
  inline void setBacklight(uint8_t c) { lcd.setBacklight(HIGH == c ? HIGH : LOW); }

  inline void print(const char *s) { lcd.print(s); }
  inline void print(int x, int y, const char *s) { lcd.setCursor(x,y); lcd.print(s); }
  void print(int y, const char *s);

  void print_p(PGM_P s);
  void print_p(int x, int y, PGM_P s);
  void print_p(int y, PGM_P s);

  inline void setCursor(int x, int y) { lcd.setCursor(x,y); }
  void clearLine(int y);
  void clear() { lcd.clear(); }

  inline void write(uint8_t data) { lcd.write(data); }
  inline void msg(const char *l1,const char *l2) { print(0,l1); print(1,l2); }
  inline void msg_p(PGM_P l1,PGM_P l2) { print_p(0,l1); print_p(1,l2); }
};

extern LCD1602 lcd1602;

#endif // _LCD1602_H_