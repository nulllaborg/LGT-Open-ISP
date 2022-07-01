//
// author:
//    Ben-An Chen @ kinetos.de
//
#include "LCD1602.h"

#include "BtnModeMenu.h"

//
// callbacks
//
void btnStartEvent() __attribute__((weak));
void menuExitEvent() __attribute__((weak));

//
// Button
//
#define BTN_STATE_OFF       0
#define BTN_STATE_SHORT     1     // short press
#define BTN_STATE_LONG      2     // long press
#define BTN_PRESS_SHORT     50    // ms
#define BTN_PRESS_LONG      500   // ms
#define BTN_PRESS_VERYLONG  10000 // ms

Btn::Btn()
{
  buttonState = BTN_STATE_OFF;
  lastDebounceTime = 0;
  vlongDebounceTime = 0;
}

void Btn::init(uint8_t btn)
{
  button = btn;
  pinMode(button, INPUT_PULLUP);  // active LOW
}

void Btn::read()
{
  uint8_t sample = digitalRead(button) ? 0 : 1; // 1 = pressed, 0 = released

  unsigned long delta;
  if (!sample && (buttonState == BTN_STATE_LONG) && !lastDebounceTime) {
    buttonState = BTN_STATE_OFF;
  }

  if ((buttonState == BTN_STATE_OFF) ||
     ((buttonState == BTN_STATE_SHORT) && lastDebounceTime)) {
    if (sample) {
      if (!lastDebounceTime && (buttonState == BTN_STATE_OFF)) {
        lastDebounceTime = millis();
      }
      delta = millis() - lastDebounceTime;

      if (buttonState == BTN_STATE_OFF) {
        if (delta >= BTN_PRESS_SHORT)
          buttonState = BTN_STATE_SHORT;
      }
      else if (buttonState == BTN_STATE_SHORT) {
        if (delta >= BTN_PRESS_LONG)
          buttonState = BTN_STATE_LONG;
      }
    }
    else
      lastDebounceTime = 0;
  }
  else if (sample && vlongDebounceTime && (buttonState == BTN_STATE_LONG)) {
    if ((millis() - vlongDebounceTime) >= BTN_PRESS_VERYLONG) {
      vlongDebounceTime = 0;
      // DoVerylongPress()
    }
  }
}

uint8_t Btn::shortPress()
{
  if ((buttonState == BTN_STATE_SHORT) && !lastDebounceTime) {
    buttonState = BTN_STATE_OFF;
    return 1;
  }
  else
    return 0;
}

uint8_t Btn::longPress()
{
  if ((buttonState == BTN_STATE_LONG) && lastDebounceTime) {
    vlongDebounceTime = lastDebounceTime;
    lastDebounceTime = 0;
    return 1;
  }
  else
    return 0;
}

//
// Mode Menu
//
static const char *modeMenuItems[] = {
  "LGT-NANO AS ISP",  // upload using programmer LGTISP, for avrdude
  "UPLOAD FIRMWARE",  // upload firmware.hex on SD card using optiboot in slave as ISP
  "BURN BOOTLOADER",  // upload bootload.hex on SD using built-in LGTDUDE
  "UPLOAD COMBINED",  // upload combined.hex on SD using built-in LGTDUDE
  " MERGE COMBINED"   // merge firmware.hex plus bootload.hex to combined.hex on SD
};

void ModeMenu::init()
{
  m_CurIdx = (uint8_t)mode;
  lcd1602.print_p(0, m_Title);
  char sTmp[17];
  sprintf(sTmp, "+%s", modeMenuItems[m_CurIdx]);
  lcd1602.print(1, sTmp);
}

void ModeMenu::next() {
  m_CurIdx++;
  if (m_CurIdx > 4) m_CurIdx = 0;
  lcd1602.clearLine(1);
  lcd1602.setCursor(0, 1);
  lcd1602.print("+");
  lcd1602.print(modeMenuItems[m_CurIdx]);
}

Menu *ModeMenu::select() {
  lcd1602.print(0, 1, "+");
  lcd1602.print(modeMenuItems[m_CurIdx]);
  mode = (Mode_t)m_CurIdx;
  return NULL;
}

//
// Button Handler
//
void BtnHandler::checkBtn() {
  m_Btn.read();
  if (m_Btn.shortPress())
    doShortPress();
  else if (m_Btn.longPress())
    doLongPress();
}

void BtnHandler::doShortPress() {
  if (m_CurMenu) {
    m_CurMenu->next();
  }
  else {
    if (btnStartEvent) btnStartEvent();
  }
}

void BtnHandler::doLongPress()
{
  if (!m_CurMenu) {
    m_CurMenu = &modeMenu;
    m_CurMenu->init();
  }
  else {
    m_CurMenu = m_CurMenu->select();
    if (m_CurMenu) {
      m_CurMenu->init();  // submenu?
    } else {
      if (menuExitEvent) menuExitEvent();
    }
  }
}

ModeMenu modeMenu;
BtnHandler btnHandler;