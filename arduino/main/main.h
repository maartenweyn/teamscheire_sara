#ifndef MAIN_H
#define MAIN_H

#include "Arduino.h"

#define LED 2
#define BUTTON 1

#define USE_MEASUREMENT_THRESHOLD 24 //4 * 6 (can miss 3 cycles)

#define FIELD_SIZE_X      2200
#define FIELD_SIZE_Y      490
#define FIELD_SIZE_MARGIN 200

#define EEPROM_SIZE 64

typedef struct {
  int x;
  int y;
} position_t;


extern bool led_mode;
extern int ranges[];
extern int counter[];

extern position_t current_position;

void store_calibration();

#endif 
