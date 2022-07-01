// Intel Hex Parser
//
// Please note:
//
//  auto-detect first page address in a parser
//  auto-detect new segment
//  only one EOF record in a parser is allowed
//
// You can use IntelHexParser::init(uint8_t *page) to restart a new parser
//
// Author:
//    Ben-An Chen @ kinetos.de
//
#ifndef __INTELHEXPARSER__H__
#define __INTELHEXPARSER__H__

#include <Arduino.h>

class IntelHexParser
{
private:
  uint8_t *_page = nullptr;
  uint16_t _pageAddress = 0;
  uint8_t  _pageLength = 0;

  bool     _firstRecord = true;
  uint8_t  _recordType = 0;
  uint16_t _recordAddress = 0;
  uint8_t  _recordLength = 0;

  // for convert from ASCII to binary
  char _buff[5] = {0};

  uint8_t getRecordLength(char *hexline);
  uint16_t getRecordAddress(char *hexline);
  uint8_t getRecordType(char *hexline);
  uint8_t *getRecordData(char *hexline, uint8_t datalen);
  void fillPageFull();

public:
  void init(uint8_t *page);

  void parseSingleRecord(char *hexline);
  inline uint8_t *getPage() { return _page; }
  inline uint16_t getPageAddress() { return _pageAddress; }

  // ATTENTION:
  //  this function uses page as buffer so destroyes parsed page!
  //  it should be used only stand-alone
  uint8_t getSingleRecordCheckSum(char *hexline);
};

extern IntelHexParser hexParser;

#endif // __INTELHEXPARSER__H__
