#include "localization.h"
#include "main.h"


void processMeasurement() {
  led_mode != led_mode;
  digitalWrite(LED, led_mode);

  for (int i = 0; i < 6; i++)
  {
    Serial.print(int(ranges[i]/10));
    Serial.print("\t");
  }
  Serial.println();
}
