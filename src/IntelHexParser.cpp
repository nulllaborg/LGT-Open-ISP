// Intel Hex Parser
//
// author:
//    Ben-An Chen @ kinetos.de
//
#include <Arduino.h>

#include "LGTDUDE.h"
#include "IntelHexParser.h"

void pageReadyEvent() __attribute__((weak));

void IntelHexParser::init(uint8_t *page)
{
  _page = page;
  _pageAddress = 0;
  _pageLength = 0;

  _firstRecord = true;
  _recordType = 0;
  _recordAddress = 0;
  _recordLength = 0;

  memset(_page, 0xFF, 128);
}

uint8_t IntelHexParser::getRecordLength(char *hexline)
{
  _buff[0] = hexline[1];
  _buff[1] = hexline[2];
  _buff[2] = '\0';
  return strtol(_buff, 0, 16);
}

uint16_t IntelHexParser::getRecordAddress(char *hexline)
{
  _buff[0] = hexline[3];
  _buff[1] = hexline[4];
  _buff[2] = hexline[5];
  _buff[3] = hexline[6];
  _buff[4] = '\0';
  return (uint16_t)strtol(_buff, 0, 16);
}

uint8_t IntelHexParser::getRecordType(char* hexline)
{
  _buff[0] = hexline[7];
  _buff[1] = hexline[8];
  _buff[2] = '\0';
  return (uint8_t)strtol(_buff, 0, 16);
}

void IntelHexParser::fillPageFull()
{
  // fill 0xFF bytes until page full
  while (0 < _pageLength && _pageLength < 128) {
    _page[_pageLength] = 0xFF;
    _pageLength++;                    // increase available page length
  }
}

void IntelHexParser::parseSingleRecord(char *hexline)
{
  _recordType = getRecordType(hexline);

  // if record type is data
  if (_recordType == 0) {
    _recordAddress = getRecordAddress(hexline);
    _recordLength = getRecordLength(hexline);

    // auto-detect first page address in parser
    if (_pageLength == 0 && _firstRecord) {
      _pageAddress = _recordAddress;  // init first page address
      _firstRecord = false;           // clear flag
    }

    // auto-detect new segment record
    if (_recordAddress > _pageAddress + 128) {
      fillPageFull();
      // firstly handle page ready event
      if (pageReadyEvent) pageReadyEvent();
      _pageLength = 0;                // clear available page length
      _pageAddress = _recordAddress;  // set page address to the new segment record address
    }

    // convert record raw data from ASCII char string to byte array
    // fill page[_pageLength, .., _pageLength+_pageLength] with byte data
    uint8_t start = 9;
    uint8_t end = start + (_recordLength * 2);

    for (uint8_t x = start; x < end; x += 2) {
      // page ready
      if (_pageLength == 128) {
        // firstly handle page ready event
        if (pageReadyEvent) pageReadyEvent();
        _pageLength = 0;              // clear available page length
        _pageAddress += 128;          // increase page address
      }
      _buff[0] = hexline[x];
      _buff[1] = hexline[x+1];
      _buff[2] = '\0';
      _page[_pageLength] = strtol(_buff, 0, 16);
      _pageLength++;                  // increase available page length
    }
  }

  // if record type is EOF
  else if (_recordType == 1) {
    fillPageFull();
    // firstly handle page ready event
    if (pageReadyEvent) pageReadyEvent();
    _pageLength = 0;                  // clear available page length
    _pageAddress += 128;              // increase page address
  }

  // other record type is ignored
  // don't parse this hexline
}

// ATTENTION:
//  this function uses page as buffer so destroyes parsed page!
//  it should be used only stand-alone
//
// hexline e.g. ":100000000C94003A0C94121B0C94EB1A0C946201A1"
// sumcheck e.g. "100000000C94003A0C94121B0C94EB1A0C946201"
// byte buf e.g. 10 00 00 00 0C 94 00 3A 0C 94 12 1B 0C 94 EB 1A 0C 94 62 01
// Checksum e.g. A1
//
uint8_t IntelHexParser::getSingleRecordCheckSum(char *hexline)
{
  uint8_t rectype = getRecordType(hexline);
  uint16_t recaddr = getRecordAddress(hexline);
  uint8_t datalen = getRecordLength(hexline);
  _pageLength = 0; 
  for (uint8_t x = 9; x < 9 + (datalen * 2); x += 2) {
    _buff[0] = hexline[x];
    _buff[1] = hexline[x+1];
    _buff[2] = '\0';
    _page[_pageLength] = strtol(_buff, 0, 16);
    _pageLength++;
  }
  uint8_t checkSum = datalen + highByte(recaddr) + lowByte(recaddr) + rectype;
  for (uint8_t i = 0; i < datalen; i++) {
    checkSum += _page[i];
  }
  // 2's complement
  checkSum = ~checkSum + 1;
#ifdef ENABLE_DEBUG
  Serial.print(F("Hex line: ")); Serial.println(hexline);
  Serial.print(F("Address: 0x")); Serial.println(recaddr, HEX);
  Serial.print(F("Byte count: ")); Serial.println(datalen);
  Serial.print(F("Record type: ")); Serial.println(rectype);
  Serial.print(F("Calculated checksum: 0x")); Serial.println(checkSum, HEX);
#endif // ENABLE_DEBUG
  return checkSum;
}

IntelHexParser hexParser;