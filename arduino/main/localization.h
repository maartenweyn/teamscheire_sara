#include "main.h"

extern position_t node_positions[];
extern position_t current_position;
extern int connected_anchors;
extern int last_position_counter;

bool processMeasurement();
void setCalibration(char letter);
