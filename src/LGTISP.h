// author : brother_yan
//
// mod for LGTISP & LGTDUDE (3in1):
//    Ben-An Chen @ kinetos.de
//
#ifndef __LGTISP_H__
#define __LGTISP_H__

#include <Arduino.h>

extern uint8_t error;
extern uint8_t buff[128];

extern int lgtisp();

extern void prog_lamp(int state); // active LOW

#endif // __LGTISP_H__