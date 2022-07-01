// LGTISP & LGTDUDE (5in1)
//
// an open source hardware and software for the LGT8F328P community
//
// (C) 2022, Ben-An Chen
//
#ifndef __PINOUT_H__
#define __PINOUT_H__

#include <Arduino.h>

// NANO pinout
#define BTN_PIN       2

#define RESET_PIN     5

// working software SWDIF pins with SD card
//#define SWDIF_CLK   4  // 4/D4/PD4/SWDIF_CLK
//#define SWDIF_DAT   7  // 7/D7/PD7/SWDIF_DAT

// working software Serial
#define SWSER_RXD     6
#define SWSER_TXD     8

#define BUZZER_PIN    9

// working SD pins
#define SD_CS         10  // 10/D10/SS/PB2/
#define SD_MOSI       11  // 11/D11/MOSI/PB3
#define SD_MISO       12  // 12/D12/MISO/PB4
#define SD_SCK        13  // 13/D13/SCK/PB5

#define LED_ERR       NOT_A_PIN   // 14/A0
#define LED_HB        NOT_A_PIN   // 15/A1
#define LED_PMODE     BTN_PIN     // shared with button pin, active LOW

#endif // __PINOUT_H__
