#ifndef APP_LEDS_H
#define APP_LEDS_H

#include <stdint.h>

void leds_init();
void leds_setcolor(uint8_t div, uint8_t r, uint8_t g, uint8_t b);


#endif