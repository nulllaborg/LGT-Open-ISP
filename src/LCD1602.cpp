// LCD1602 I2C
//
// author:
//    Ben-An Chen @ kinetos.de
//
#include "LCD1602.h"

void LCD1602::print(int y, const char *s)  // print at (0,y), filling out the line with trailing spaces
{
  lcd.setCursor(0, y);
  uint8_t i, len = strlen(s);
  if (len > LCD_MAX_CHARS_PER_LINE)
    len = LCD_MAX_CHARS_PER_LINE;
  for (i = 0; i < len; i++) {
    lcd.write(s[i]);
  }
  for (i = len; i < LCD_MAX_CHARS_PER_LINE; i++) {
    lcd.write(' ');
  }
}

void LCD1602::print_p(PGM_P s)
{
  strncpy_P(strBuf,s,LCD_MAX_CHARS_PER_LINE);
  strBuf[LCD_MAX_CHARS_PER_LINE] = 0;
  lcd.print(strBuf);
}
void LCD1602::print_p(int x,int y,PGM_P s)
{
  strncpy_P(strBuf,s,LCD_MAX_CHARS_PER_LINE);
  strBuf[LCD_MAX_CHARS_PER_LINE] = 0;
  lcd.setCursor(x,y);
  lcd.print(strBuf);
}
void LCD1602::print_p(int y,PGM_P s)
{
  strncpy_P(strBuf,s,LCD_MAX_CHARS_PER_LINE);
  strBuf[LCD_MAX_CHARS_PER_LINE] = 0;
  print(y,strBuf);
}

void LCD1602::clearLine(int y)
{
  lcd.setCursor(0,y);
  for (uint8_t i=0;i < LCD_MAX_CHARS_PER_LINE;i++) {
    lcd.write(' ');
  }
  lcd.setCursor(0,y);
}

LCD1602 lcd1602;