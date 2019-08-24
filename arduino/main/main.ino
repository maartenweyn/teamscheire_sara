
#include "uwb_parser.h"
#include "localization.h"
#include "main.h"
#include "webserver.h"


#include <EEPROM.h>
#include <WiFi.h>




int ranges[6] = {-1,-1,-1,-1,-1,-1};
int counter[6] = {100,100,100,100,100,100};
bool led_mode = 0;

enum state_codes { entry, calibration, localization, end};
enum calibration_state_codes { go_to_m, cal_m, goto_h, cal_h, goto_k, cal_k, goto_f, cal_f, finished};

enum state_codes cur_state = entry;
enum calibration_state_codes cal_state = go_to_m;

extern WiFiServer server;

void store_calibration() {

  Serial.println("store_calibration");
  byte* value = (byte*)&node_positions;
  
  for (int i=0; i < sizeof(position_t) * 6; i++) {
    Serial.print(*value, DEC);
    EEPROM.write(i, *value);
    EEPROM.commit();
    value++;
  }



  Serial.println();

  load_calibration();
}

void load_calibration() {

  Serial.println("load_calibration");
  byte* value = (byte*)&node_positions;
  
  for (int i=0; i < sizeof(position_t) * 6; i++) {
    *value = EEPROM.read(i);
    Serial.print(*value, DEC);
    value++;
  }

  Serial.println();
}

void setup() {
  
  pinMode(LED, OUTPUT);   // Status LED ESP
  Serial.begin(115200);   // console

  uwb_parser_init();

  
  //pinMode(BUTTON, INPUT);    // declare pushbutton as input

//  byte value = EEPROM.read(0);
//  Serial.print("eeprom 0:");
//  Serial.print(value, DEC);
//  Serial.println();
//
//  value++;
//  EEPROM.write(0, value);

  // CHECK if calibrated, if so go to localization, else ENTRY_STATE
  //Serial.println("Not Calibrated");

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM");
  }

  load_calibration();

  webserver_setup();
}

void give_command_calibration(calibration_state_codes cal_state) {
  switch (cal_state)
    {
      case go_to_m:
        Serial.println("goto_m");
        break;
    }
}



void loop() {
//  String command = "";
//  if (Serial.available() > 0) {
//      String command = Serial.readString();
//  }
//  
//  if (cur_state == entry) {
//    if (command.equals("cal")) {
//      cur_state = calibration;
//      cal_state = go_to_m;
//    }
//  } else if (cur_state == calibration) {
//    switch (cal_state)
//    {
//      case go_to_m:
//        if (command == "1") {
//          cal_state = cal_m;
//          break;
//        }
//        give_command_calibration(cal_state);
//        delay(1000);
//        break;
//      case cal_m:
//        break;
//    }
//  }
  
//  bool new_position = uwb_parser_check_data();
//
//  if (new_position) {
//    Serial.print("New Position: ");
//    Serial.print(current_position.x);
//    Serial.print(", ");
//    Serial.println(current_position.y);
//  }

  webserver_check_for_client();


}
