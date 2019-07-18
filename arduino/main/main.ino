
#include "uwb_parser.h"
#include "localization.h"
#include "main.h"


#include <EEPROM.h>




int ranges[6] = {-1,-1,-1,-1,-1,-1};
int counter[6] = {-1,-1,-1,-1,-1,-1};
bool led_mode = 0;

void setup() {
  
  pinMode(LED, OUTPUT);   // Status LED ESP
  Serial.begin(115200);   // console

  byte value = EEPROM.read(0);
  Serial.print("eeprom 0:");
  Serial.print(value, DEC);
  Serial.println();

  value++;
  EEPROM.write(0, value);
}




void loop() {
  uwb_parser_check_data();

}
