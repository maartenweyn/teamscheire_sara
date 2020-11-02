#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "main.h"


#include <stdbool.h>
#include <stdint.h>

typedef struct {
  char letter;
  int ranges[6];
  int count;
} calibration_ranges_t;

typedef struct {
   int16_t x;
   int16_t y;
   int16_t speed; //cm/s
   float weight;
   float direction; // radians
} particle_t;




extern localization_result_t current_position;
extern int connected_anchors;
extern int last_position_counter;
extern int nearby_letter;
extern bool receiving_ranges;

bool processMeasurement();
void initialize_localization_engine();
void setCalibration(char letter);

void locator_task( void *pvParameters );

#endif