//
// author:
//    Ben-An Chen @ kinetos.de
//
#ifndef __BTN_MODE_MENU_H__
#define __BTN_MODE_MENU_H__

#include <Arduino.h>

//
// Button
//
class Btn
{
private:
  uint8_t button;
  uint8_t buttonState;
  unsigned long lastDebounceTime;  // the last time the output pin was toggled
  unsigned long vlongDebounceTime; // for verylong press

public:
  Btn();
  void init(uint8_t btn);
  void read();
  uint8_t shortPress();
  uint8_t longPress();
};

//
// Menu
//
class Menu
{
public:
  PGM_P m_Title;
  Menu() {}
  virtual void init() = 0;
  virtual void next() = 0;
  virtual Menu *select() = 0;
};

//
// Mode Menu
//
typedef enum {
  MODE_LGTISP,      // upload using programmer LGTISP for avrdude
  MODE_FIRMWARE,    // upload firmware.hex on SD card using optiboot in slave as ISP
  MODE_BOOTLOADER,  // upload bootload.hex on SD using built-in LGTDUDE
  MODE_COMBINED,    // upload combined.hex on SD using built-in LGTDUDE
  MODE_MERGE        // merge firmware.hex plus bootload.hex to combined.hex on SD
} Mode_t;

class ModeMenu : public Menu
{
private:
  uint8_t m_CurIdx = 0;

public:
  Mode_t mode;
  ModeMenu() { m_Title = PSTR("UPLOADERER PMODE"); mode = MODE_LGTISP; }
  void init();
  void next();
  Menu *select();
};

//
// Button Handler
//
class BtnHandler {
  Btn m_Btn;
  Menu *m_CurMenu;

public:
  BtnHandler() { m_CurMenu = NULL; }
  void init(uint8_t btn) { m_Btn.init(btn); }
  void checkBtn();
  void doShortPress();
  void doLongPress();
};

extern ModeMenu modeMenu;
extern BtnHandler btnHandler;

#endif // __BTN_MODE_MENU_H__