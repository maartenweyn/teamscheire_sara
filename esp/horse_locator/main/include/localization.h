#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "main.h"


#include <stdbool.h>

//#define NR_OF_LETTERS 11
#define NR_OF_LETTERS 11

typedef struct {
  char letter;
  int ranges[6];
  int count;
} calibration_ranges_t;

typedef struct {
  char letter;
  int x;
  int y;
} letter_position_t;

extern letter_position_t letters[NR_OF_LETTERS];
extern position_t current_position;
extern int connected_anchors;
extern int last_position_counter;
extern int nearby_letter;
extern bool receiving_ranges;

bool processMeasurement();
void setCalibration(char letter);

void locator_task( void *pvParameters );

#endif