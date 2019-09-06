#include "main.h"

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

extern position_t node_positions[];
extern letter_position_t letters[];
extern position_t current_position;
extern int connected_anchors;
extern int last_position_counter;
extern int nearby_letter;

bool processMeasurement();
void setCalibration(char letter);
