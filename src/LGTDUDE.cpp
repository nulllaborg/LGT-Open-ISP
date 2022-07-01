// an avrdude, ported for lgt8f328p
//
// author:
//    Ben-An Chen @ kinetos.de
//
#include "pinout.h"
#include "BtnModeMenu.h"
#include "LCD1602.h"
#include "Buzzer.h"

#include "LGTISP.h"
#include "swd_lgt8fx8p.h"
#include "stk500.h"

#include "LGTDUDE.h"
#include <SoftwareSerial.h>
#include "IntelHexParser.h"

SoftwareSerial swSerial(SWSER_RXD, SWSER_TXD);

void LGTDUDEClass::send_cmd(uint8_t b)
{
  swSerial.write(b);
  swSerial.write(CRC_EOP);
}

uint8_t LGTDUDEClass::getch() // wait max 1000 ms
{
  int i;
  for (i = 0; !swSerial.available() && (i < 1000); i++) { delay(1); }
  if (i == 1000) { return 0x00; }
  return swSerial.read();
}

void LGTDUDEClass::verify_reply()
{
  uint8_t sync = getch();
  uint8_t ok = getch();
  //Serial.print("sync:0x"); Serial.print(sync, HEX); Serial.print(", ok:0x"); Serial.println(ok, HEX);
  if (STK_INSYNC != sync || STK_OK != ok)
    error = BEEP_DONE_NOK;
}

bool LGTDUDEClass::begin()
{
  // init Software Serial
  swSerial.begin(115200);

  // init RESET pin
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);

  // init Intel Hex Parser
  hexParser.init(buff);

  // reset MCU
  digitalWrite(RESET_PIN, LOW);
  delay(1);
  digitalWrite(RESET_PIN, HIGH);
  delay(100);
  digitalWrite(RESET_PIN, LOW);
  delay(1);
  digitalWrite(RESET_PIN, HIGH);
  delay(100);

  // get Sync
  send_cmd(STK_GET_SYNC);
  verify_reply();
  if (error) {
    error = BEEP_ERR_ENTER_PMODE;
    return false;
  }

  // set Device
  // STK_SET_DEVICE is ignored by optiboot_lgt8f328p, do nothing

  // set Device Ext
  // STK_SET_DEVICE_EXT is ignored by optiboot_lgt8f328p, do nothing

  // enter Prog Mode
  send_cmd(STK_ENTER_PROGMODE);
  verify_reply();
  if (error) {
    error = BEEP_ERR_ENTER_PMODE;
    return false;
  }

  // read Signature
  send_cmd(STK_READ_SIGN);
  uint8_t sync = getch();
  uint8_t b[3];
  for (int x = 0; x < 3; x++)
    b[x] = getch();
  uint8_t ok = getch();

  // check Signature
  if (STK_INSYNC  == sync &&
      SIGNATURE_0 == b[0] &&
      SIGNATURE_1 == b[1] &&
      SIGNATURE_2 == b[2] &&
      STK_OK      == ok)
  {
    pmode = 1;
  }
  else {
    pmode = 0;
    error = BEEP_ERR_ENTER_PMODE;
  }

  return pmode;
}

void LGTDUDEClass::end()
{
  uint8_t lastError = 0;
  if (error) lastError = error;

  // leave Prog Mode
  send_cmd(STK_LEAVE_PROGMODE);
  verify_reply();

  if (!lastError) {
    if (error) {
      error = BEEP_ERR_LEAVE_PMODE;
    }
  }
  else {
    error = lastError;
  }
  pmode = 0;

  // set RESET pin back to input
  pinMode(RESET_PIN, INPUT_PULLUP);
}

void LGTDUDEClass::init()
{
  // init Intel Hex Parser
  hexParser.init(buff);
}

void LGTDUDEClass::write_flash_pages(uint16_t addr, uint8_t buf[], int size)
{
  // load address
  swSerial.write(STK_LOAD_ADDRESS);
  swSerial.write(lowByte(addr));
  swSerial.write(highByte(addr));
  swSerial.write(CRC_EOP);
  verify_reply();
  if (error) { error = BEEP_ERR_PROG_PAGE; return; }

  // write flash page
  swSerial.write(STK_PROG_PAGE);
  swSerial.write(highByte(size)); // length is big-endian
  swSerial.write(lowByte(size));
  swSerial.write((uint8_t)0x46);  // 'F'lash
  for (int x = 0; x < size; x++) {
    swSerial.write(buf[x]);
  }
  swSerial.write(CRC_EOP);
  verify_reply();
  if (error) { error = BEEP_ERR_PROG_PAGE; }
}

void LGTDUDEClass::read_flash_page(uint16_t addr, uint8_t buf[], int size, uint8_t *verify)
{
  // load address
  swSerial.write(STK_LOAD_ADDRESS);
  swSerial.write(lowByte(addr));
  swSerial.write(highByte(addr));
  swSerial.write(CRC_EOP);
  verify_reply();
  if (error) { error = BEEP_ERR_READ_PAGE; return; }

  // read flash page
  swSerial.write(STK_READ_PAGE);
  swSerial.write(highByte(size)); // length is big endian
  swSerial.write(lowByte(size));
  swSerial.write((uint8_t)0x46);  // 'F'lash
  swSerial.write(CRC_EOP);
  uint8_t sync = getch();
  for (int x = 0; x < size; x++) {
    if (verify && buf[x] != getch()) error++;
    else if (!verify) buf[x] = getch();
  }
  uint8_t ok = getch();
  if (STK_INSYNC != sync || error != 0 || STK_OK != ok) { error = BEEP_ERR_READ_PAGE; }
}

bool LGTDUDEClass::write(uint16_t addr, uint8_t buf[], int size)
{
  if (pmode)
    write_flash_pages(addr, buf, size);
  return error ? false : true;
}

bool LGTDUDEClass::read(uint16_t addr, uint8_t buf[], int size, uint8_t *verify)
{
  if (pmode)
    read_flash_page(addr, buf, size, verify);
  return error ? false : true;
}

void LGTDUDEClass::parseLine(char *hexline)
{
  hexParser.parseSingleRecord(hexline);
}

void LGTDUDEClass::pageReadyHandler()
{
  prog_lamp(LOW); // turn ON

  // get page's byte address
  uint16_t addr = hexParser.getPageAddress();
  // get page buffer
  uint8_t *page = hexParser.getPage();

  // handle event
  if (MODE_FIRMWARE == modeMenu.mode) {
    // convert from byte address to word address
    addr /= 2;
#ifdef ENABLE_DEBUG
    Serial.print(F("word"));
#endif // ENABLE_DEBUG
    // writing flash ...
    write(addr, page, 128);
    // reading on-chip flash data, verifying ...
    read(addr, page, 128, &error);
  } else {  // BOOTLOADER || COMBINED
    // using byte address
#ifdef ENABLE_DEBUG
    Serial.print(F("byte"));
#endif // ENABLE_DEBUG
    // writing flash ...
    LGTISP.write(addr, page, 128);
    // reading on-chip flash data, verifying ...
    LGTISP.read(addr, page, 128, &error);
  } // BOOTLOADER || COMBINED

#ifdef ENABLE_DEBUG
  char s[16];
  sprintf(s, " addr: 0x%04X\n", addr);
  Serial.print(s);
  for (int x = 0; x < 128; x++) {
    sprintf(s, " %02X", page[x]);
    Serial.print(s);
    if (x % 16 == 15) Serial.println();
  }
  if (error) { Serial.print(F("verify err cnt: ")); Serial.println(error); }
#endif // ENABLE_DEBUG

  if (error) {
    error = BEEP_ERR_PROG_PAGE;
  }
  // else
  //   hexParser.incLoadAddress();

  prog_lamp(HIGH); // turn OFF
}

LGTDUDEClass LGTDUDE;