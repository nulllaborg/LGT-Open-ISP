// LGTISP & LGTDUDE (5in1)
//
// an open source hardware and software for the LGT8F328P community
//
// (C) 2022, Ben-An Chen
//
// Five working modes for LGT8F328P:
//
//  LGTISP  (default)       hardware serial <-> lgtisp  <-> software swd
//  LGTDUDE (FIRMWARE)      SD firmware.hex --> lgtdude <-> software serial  (hardware serial as console)
//  LGTDUDE (BOOTLOADER)    SD bootload.hex --> lgtdude <-> software swd     (hardware serial as console)
//  LGTDUDE (COMBINED)      SD combined.hex --> lgtdude <-> software swd     (hardware serial as console)
//   (MERGE COMBINED)       SD firmware.hex --> merge   <-> combined.hex     (hardware serial as console)
//                          SD bootload.hex --> merge   <-> combined.hex     (hardware serial as console)
//
// Beep tones:
//
//  BEEP_START              1
//  BEEP_DONE_OK            2
//  BEEP_DONE_NOK           3
//  BEEP_ERR_ENTER_PMODE    4
//  BEEP_ERR_OPEN_FILE      5
//  BEEP_ERR_PROG_PAGE      6
//  BEEP_ERR_READ_PAGE      7
//  BEEP_ERR_LEAVE_PMODE    8
//
// Please note:
//
//  AVRISP programmer:      avrdude uses word address to communicate with AVRISP programmer, AVRISP writes flash over ICSP
//                          optiboot in an AVR chip is an Arduino ISP programmer
//
//  LGTISP programmer:      avrdude uses word address to communicate with LGTISP programmer, LGTISP writes flash over SWD
//                          optiboot in an LGT chip is an Arduino ISP programmer
//                          (1) convert from word address to byte address in write_flash()
//                          (2) convert from byte address to word32 address in write_flash_pages()
//                          (3) LGTISP writes 4 bytes at given address with SWD_EEE_Write(uint32_t data, uint16_t addr)
//
//  LGTDUDE FIRMWARE:       lgtdude uses word address to communicate with optiboot in an LGT chip, optiboot in chip writes flash
//                          optiboot in an LGT chip is an Arduino ISP programmer
//                          (a) read hexline from firmware.hex, parse the hexline, load address (byte) when page ready
//                          (b) convert from byte address to word address in pageReadyHandle()
//                          (c) call LGTDUDE.write(), using word address to write_flash_pages() over optiboot
//
//  LGTDUDE BOOTLOADER:     lgtdude uses byte address to communicate with LGTISP programmer, LGTISP writes flash over SWD
//                          (A) read hexline from bootload.hex, parse the hexline, load address (byte) when page ready
//                          (B) call LGTISP.write(), using byte address to write_flash_pages() in (2)
//
//
//  LGTDUDE COMBINED:       lgtdude uses byte address to communicate with LGTISP programmer, LGTISP writes flash over SWD
//                          (I) read hexline from combined.hex, parse the hexline, auto detect
//                              byte address of FIRMWARE and BOOTLOADER, generate pages separatly, load address (byte) when page ready
//                          (II) call LGTISP.write(), using byte address to write_flash_pages() in (2)
//
//  MERGE COMBINED:         merge firmware.hex and bootload.hex to one combined.hex
//                          live-patch the firmware.hex into combined.hex so the bootloader runs
//                          overwrite combined.hex if it exists on SD
//
#include <Arduino.h>
#include <SD.h>             // Note: SD library needs 512 bytes free RAM to run. I have tested it with 330 bytes free RAM, it runs

#include "pinout.h"
#include "LCD1602.h"
#include "BtnModeMenu.h"
#include "Buzzer.h"

#include "LGTISP.h"
#include "swd_lgt8fx8p.h"
#include "LGTDUDE.h"
#include "IntelHexParser.h"

uint8_t lcdP = 0;
uint8_t lcdE = 0;
bool start = false;

const char firmwareHex[] = "FIRMWARE.HEX";
const char bootloadHex[] = "BOOTLOAD.HEX";
const char combinedHex[] = "COMBINED.HEX";

char hexline[48] = {0};

void lcd1602_show() {
  lcd1602.clear();
  lcd1602.print_p(0,                                      PSTR("LGTISP & LGTDUDE"));
  lcd1602.print_p(1,  MODE_LGTISP     == modeMenu.mode ?  PSTR("     LGTISP") :
                      MODE_FIRMWARE   == modeMenu.mode ?  PSTR("    FIRMWARE") :
                      MODE_BOOTLOADER == modeMenu.mode ?  PSTR("   BOOTLOADER") :
                      MODE_COMBINED   == modeMenu.mode ?  PSTR("    COMBINED") :
                      MODE_MERGE      == modeMenu.mode ?  PSTR(" MERGE COMBINED") :
                                                          PSTR(" "));
}

void btnStartEvent() {
  if (MODE_LGTISP != modeMenu.mode && !LGTISP.isPmode() && !start)
  {
    if (error) {                          // in fault
      error = 0; lcdE = 0;                // manual override
      lcd1602.print_p(15, 1, PSTR(" "));  // clear LCD 'E'
    }
    // BTN_PIN and LED_PMODE is a shared pin
    // now the BTN_PIN pin is not is use, so set pin mode to OUTPUT for LED_PMODE
    pinMode(LED_PMODE, OUTPUT);
    start = true;
  }
}

void menuExitEvent() {
  lcd1602_show();
}

void pageReadyEvent() {
  LGTDUDE.pageReadyHandler();
}

int fileReadLineUntilCRLF(File *file, char *line, int size)
{
  memset(line, 0, size);
  int len = 0;
  int ch = file->read();
  while ((len < size) && (ch >= 0) && (ch != '\n')) {
    if (ch != '\r') line[len++] = (char)ch;
    ch = file->read();
  }
  return len;
}

uint8_t h2b(char h0, char h1)
{
  char b[3] = {0};
  b[0] = h0;
  b[1] = h1;
  return strtol(b, 0, 16);
}

void lgtdude()
{
  // enter pmode
  if (MODE_FIRMWARE == modeMenu.mode) {
    LGTDUDE.begin();  // init hex parser, init swSerial, init RESET pin, reset MCU, start_pmode() via swSerial
  }
  else { // BOOTLOADER || COMBINED
    LGTDUDE.init();   // init hex parser only
    LGTISP.begin();   // start_pmode() via SWD
  }

#ifdef ENABLE_DEBUG
  Serial.print(F("et pm 0x")); Serial.println(pmode, HEX);
#endif // ENABLE_DEBUG

  if (pmode)
  {
    // open hex file
#ifdef ENABLE_DEBUG
    Serial.println(F("op f"));
#endif // ENABLE_DEBUG
    File hexfile;
    if (MODE_FIRMWARE == modeMenu.mode) {
      hexfile = SD.open(firmwareHex);
    }
    else if (MODE_BOOTLOADER == modeMenu.mode) {
      hexfile = SD.open(bootloadHex);
    }
    else if (MODE_COMBINED == modeMenu.mode) {
      hexfile = SD.open(combinedHex);
    }
    else {
      hexfile = File(); // empty object
    }

    if (hexfile)
    {
      lcd1602.print_p(0, 1, PSTR("P"));
      buzzer.beep(BEEP_START);  // 1x beeps OK enter progmode
      delay(500);

      int linecount = 0;
      while (hexfile.available())
      {
        int len = fileReadLineUntilCRLF(&hexfile, hexline, sizeof(hexline));

        // ignore invalid lines
        if (len > 10 && hexline[0] == ':')
        {
#ifdef ENABLE_DEBUG
          //Serial.print("l cnt:"); Serial.print(linecount); Serial.print(", len:"); Serial.println(len);
#endif // ENABLE_DEBUG
          if (MODE_FIRMWARE == modeMenu.mode)
          {
            // parse each line
            LGTDUDE.parseLine(hexline);
          } // end of FIRMWARE

          else // BOOTLOADER || COMBINED
          {
            // check first line, is it the line of an optiboot's reset vector?
            if (MODE_BOOTLOADER == modeMenu.mode && linecount == 0)
            {
              if (hexline[1]  == '0' && hexline[2]  == '4' &&
                  hexline[3]  == '0' && hexline[4]  == '0' && hexline[5]  == '0' && hexline[6]  == '0' &&
                  hexline[7]  == '0' && hexline[8]  == '0' && len == 19)
              {
#ifdef ENABLE_DEBUG
                Serial.println(F("bl")); // OK, it's the first line of our optiboot_lgt8f328p_115200_int32M.hex
#endif // ENABLE_DEBUG
                // write this optiboot's reset vector (a DWORD) to flash at address 0x0000
                buff[0] = h2b(hexline[9],  hexline[10]);
                buff[1] = h2b(hexline[11], hexline[12]);
                buff[2] = h2b(hexline[13], hexline[14]);
                buff[3] = h2b(hexline[15], hexline[16]);
                if (!LGTISP.write(0, buff, 4)) {
                  error = BEEP_DONE_NOK;
                }
              }
              else {
#ifdef ENABLE_DEBUG
                Serial.println(F("no bl"));
#endif // ENABLE_DEBUG
                error = BEEP_DONE_NOK;
                break;
              }
            }

            // otherwise linecount > 0, so we parse this hexline,
            // but ignore the line with record type '03 - start segment address'
            else
            {
              LGTDUDE.parseLine(hexline);
            }
          } // end of BOOTLOADER || COMBINED

          if (error) break;
          linecount++;
        } // end of if (hexline[0] == ':' && len > 12)
      } // end of while (hexfile.available())

      // close hexfile
#ifdef ENABLE_DEBUG
      Serial.println(F("cl f"));
#endif // ENABLE_DEBUG
      hexfile.close();
    } // end of if (hexFile)
    else {
#ifdef ENABLE_DEBUG
      Serial.println(F("op f err"));
#endif // ENABLE_DEBUG
      error = BEEP_ERR_OPEN_FILE;
    }
  } // end of if (pmode)
  else {
    error = BEEP_ERR_ENTER_PMODE;
  }

  // leave progmode
  if (MODE_FIRMWARE == modeMenu.mode) {
    LGTDUDE.end();
  } else { // BOOTLOADER || COMBINED
    LGTISP.end();
  } // BOOTLOADER || COMBINED
#ifdef ENABLE_DEBUG
  Serial.print(F("lv pm 0x")); Serial.println(pmode, HEX);
#endif // ENABLE_DEBUG
  lcd1602.print_p(0, 1, PSTR(" "));

  // done
  start = false;
  if (!error) {
    buzzer.beep(BEEP_DONE_OK);
  }
}

void mergingCombined()
{
  const char *fn = firmwareHex; // we start merging with firmware
  again:

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File f = SD.open(fn);
#ifdef ENABLE_DEBUG
  Serial.print(F("op ")); Serial.print(fn); Serial.println(f ? F(" ok") : F(" nok"));
#endif // ENABLE_DEBUG
  if (!f) { error++; return; }

  int cnt = 0;
  char rstvect[8] = {0};
  while (f.available())
  {
    int len = fileReadLineUntilCRLF(&f, hexline, sizeof(hexline));
    uint32_t pos = f.position();
    bool ignore = false;

    // ignore invalide lines
    if (len > 1 && hexline[0] == ':')
    {
      if (start) {
        // We need to live-patch the firmware so the bootloader runs
        if (cnt == 0) {
          // This is the reset vector line of firmware
          // Save RESET vector to rstvect[0..7]
          for (int x = 0; x < 8; x++) {
            rstvect[x] = hexline[9 + x];
          }

          // Add jump to bootloader at RESET vector
          // RESET vector is 8 chars from hexline[9] to hexline[16]
          hexline[ 9] = '0'; hexline[10] = 'C';
          hexline[11] = '9'; hexline[12] = '4'; // jmp
          hexline[13] = '0'; hexline[14] = '0';
          hexline[15] = '3'; hexline[16] = 'A'; // 0x7400 (0x3a00)
        }
        else if (cnt == 1) {
          // This is the WDT vector line of firmware
          // Move RESET vector from rstvect[0..7] to WDT vector
          // WDT vector is 8 chars from hexline[25] to hexline[32]
          for (int x = 0; x < 8; x++) {
            hexline[25 + x] = rstvect[x];
          }
        }
        else if (hexline[7] == '0' && hexline[8] == '1') {
          // This is the End Of File of firmware
          // We ignore it
          ignore = true;
        }

        // patch checkSum of the reset vector line and the WDT vector line in firmware
        // Checksum is 2 chars from hexline[41] to hexline[42]
        if (cnt == 0 || cnt == 1) {
          hexParser.init(buff);
          sprintf(&hexline[41], "%02X", hexParser.getSingleRecordCheckSum(hexline));
        }
      }
      else if (!start) {
        if (cnt == 0 && len == 19 && hexline[15] == '3' && hexline[16] == 'A') {
          // This is the reset vector line of bootloader
          // We ignore this line, because we have already handled the RESET vector of bootloader
          ignore = true;
        }
      }

      if (!ignore)
      {
#ifdef ENABLE_DEBUG
        Serial.print(F("cl ")); Serial.println(fn);
#endif // ENABLE_DEBUG
        f.close();

        // open combined file and immediately close it
        // overwrite combined.hex when is merging firmware and first line in firmware
        if (start && cnt == 0) {
          f = SD.open(combinedHex, O_READ | O_WRITE | O_CREAT | O_TRUNC);
          if (!f) { error++; break; }
          f.println();
          f.close();
        }
        f = SD.open(combinedHex, FILE_WRITE);
#ifdef ENABLE_DEBUG
        Serial.print(F("op ")); Serial.print(combinedHex); Serial.println(f ? F(" ok") : F(" nok"));
#endif // ENABLE_DEBUG
        if (!f) { error++; break; }

        // write hexline to combined file
        prog_lamp(LOW); // ON
        f.println(hexline);
        prog_lamp(HIGH); // OFF

#ifdef ENABLE_DEBUG
        Serial.println(hexline);
        Serial.print(F("cl ")); Serial.println(combinedHex);
#endif // ENABLE_DEBUG
        f.close();

        // re-open firmware file and immediately close it
        f = SD.open(fn);
#ifdef ENABLE_DEBUG
        Serial.print(F("op ")); Serial.print(fn); Serial.println(f ? F(" ok") : F(" nok"));
#endif // ENABLE_DEBUG
        if (!f) { error++; break; }

        bool sk = f.seek(pos);
#ifdef ENABLE_DEBUG
        Serial.print(F("sk pos ")); Serial.print(pos); Serial.println(sk ? F(" ok") : F(" nok"));
#endif // ENABLE_DEBUG
        if (!sk) { error++; break; }
      } // end of if (!ignore)
    } // end of if (len > 1 && hexline[0] == ':')

    // increase line count
    cnt++;
  } // end of while (f.available())

#ifdef ENABLE_DEBUG
  Serial.print(F("cl ")); Serial.println(fn);
#endif // ENABLE_DEBUG
  f.close();

  if (!error && start) {
    fn = bootloadHex;
    start = false;
    goto again;
  }
}

void merge()
{
  error = 0;
  lcd1602.print_p(1, PSTR("MERGING COMBINED"));
  mergingCombined();
  lcd1602.print_p(1, error ? PSTR("  MERGE FAILED") : PSTR("MERGE SUCCESSFUL"));
  buzzer.beep(error ? BEEP_DONE_NOK : BEEP_DONE_OK);
  delay(4000);
  lcd1602_show();
  error = 0;
}

void setup() {
  btnHandler.init(BTN_PIN);

  lcd1602.begin(16, 2);
  lcd1602.setBacklight(HIGH);
  lcd1602_show();

  SD.begin(SD_CS);

  Serial.begin(115200);
}

void loop() {
  // is pmode active?
  if (LGTISP.isPmode() && !lcdP) {
    lcdP = 1; // prevent re-entry
    lcd1602.print_p( 0, 1, PSTR("P"));
    buzzer.beep(BEEP_START);
  }
  else if (!LGTISP.isPmode() && lcdP) {
    lcdP = 0; // prevent re-entry
    lcd1602.print_p( 0, 1, PSTR(" "));
    if (!error) buzzer.beep(BEEP_DONE_OK);
  }

  // is an error?
  if (error && !lcdE) {
    lcdE = 1; // prevent re-entry
    lcd1602.print_p(15, 1, PSTR("E"));
    delay(500);
    buzzer.beep(error < BEEP_DONE_NOK ? BEEP_DONE_NOK : error);
  }
  else if (!error && lcdE) {
    lcdE = 0; // prevent re-entry
    lcd1602.print_p(15, 1, PSTR(" "));
  }

// #ifdef ENABLE_DEBUG
  if (start) { Serial.print(F("Free RAM ")); Serial.println(FreeRam()); }
// #endif // ENABLE_DEBUG

  switch (modeMenu.mode)
  {
  case MODE_LGTISP:     // hardware serial <-> lgtisp <-> software swd
    if (Serial.available()) {
      lgtisp();
    }
    break;

  case MODE_FIRMWARE:   // SD firmware.hex --> lgtdude <-> software serial  (hardware serial as console)
  case MODE_BOOTLOADER: // SD bootload.hex --> lgtdude <-> software swd     (hardware serial as console)
  case MODE_COMBINED:   // SD combined.hex --> lgtdude <-> software swd     (hardware serial as console)
    if (start) {
      // short pressed
      buzzer.beep(BEEP_START);
      delay(500);
      lgtdude();
    }
    break;

  case MODE_MERGE:
    if (start) {
      // short pressed
      buzzer.beep(BEEP_START);
      delay(500);
      merge();
    }
    break;

  default:
    break;
  }

  // check button when not programming
  if (!LGTISP.isPmode()) {
    // BTN_PIN and LED_PMODE is a shared pin
    // now the LED_PMODE pin is not is use, so set pin mode to INPUT_PULLUP for BTN_PIN
    pinMode(BTN_PIN, INPUT_PULLUP);
    btnHandler.checkBtn();
  }
}