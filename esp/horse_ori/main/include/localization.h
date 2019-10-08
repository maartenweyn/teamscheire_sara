#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "main.h"


#include <stdbool.h>

typedef struct {
  char letter;
  int ranges[6];
  int count;
} calibration_ranges_t;



extern position_t current_position;
extern int connected_anchors;
extern int last_position_counter;
extern int nearby_letter;
extern bool receiving_ranges;

bool processMeasurement();
void setCalibration(char letter);

void locator_task( void *pvParameters );

#endif