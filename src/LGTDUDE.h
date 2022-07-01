// an avrdude, ported for lgt8f328p
//
// author:
//    Ben-An Chen @ kinetos.de
//
#ifndef __LGTDUDE_H__
#define __LGTDUDE_H__

class LGTDUDEClass
{
private:
  void send_cmd(uint8_t b);

  uint8_t getch(); // wait max 1000 ms
  void verify_reply();

  void write_flash_pages(uint16_t addr, uint8_t buf[], int size);
  bool write(uint16_t addr, uint8_t buf[], int size);

  void read_flash_page(uint16_t addr, uint8_t buf[], int size, uint8_t *verify);
  bool read(uint16_t addr, uint8_t buf[], int size, uint8_t *verify=NULL);

public:
  void init();  // please use it if MODE_BOOTLOADER || MODE_COMBINED

  bool begin(); // please use it if MODE_FIRMWARE
  void end();   // please use it if MODE_FIRMWARE

  void parseLine(char *hexline);
  void pageReadyHandler();
};

extern LGTDUDEClass LGTDUDE;

#endif // __LGTDUDE_H__