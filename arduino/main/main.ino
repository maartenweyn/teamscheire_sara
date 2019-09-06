
#include "uwb_parser.h"
#include "localization.h"
#include "main.h"
#include "webserver.h"


#ifdef SIMULATION
#include "simulation.h"
#endif


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

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM");
  }

  load_calibration();

  webserver_setup();
}

//void give_command_calibration(calibration_state_codes cal_state) {
//  switch (cal_state)
//    {
//      case go_to_m:
//        Serial.println("goto_m");
//        break;
//    }
//}



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

  int prev_letter = nearby_letter;
  
//  

#ifdef SIMULATION
  static position_t pos = {path[0].x*100, path[0].y*100};
  static int pos_index = 1;

  delay(1000);

  double d = sqrt(pow(path[pos_index].x*100 - pos.x, 2) + pow(path[pos_index].y*100 - pos.y, 2));

  //Serial.print("pos_index " + String(pos_index)  + ": " + String(path[pos_index].x) + ", " + String(path[pos_index].y));
  //Serial.print(" -> " + String(pos.x) + ", " + String(pos.y));
  //Serial.println(" -> " + String(d) + "(" + String(SIMULATION_STEP_SIZE) + ")");
  
  if (d < SIMULATION_STEP_SIZE) {
    pos.x = path[pos_index].x*100;
    pos.y = path[pos_index].y*100;
    pos_index++;

    if (pos_index >= 27) pos_index = 0;
  } else {
    int step = d / SIMULATION_STEP_SIZE;
    pos.x += (path[pos_index].x*100 - pos.x) / step;
    pos.y += (path[pos_index].y*100 - pos.y) / step;
  }

  current_position.x = pos.x;
  current_position.y = pos.y;
  bool new_position = true;

  nearby_letter = -1;
  for (int i = 0; i < 11; i++) {
    int d = pow(letters[i].x - current_position.x, 2) + pow(letters[i].y - current_position.y, 2);
    //Serial.println("  " + String(letters[i].letter) + ": " + String(d));
    if (d < NEARBY_THRESHOLD) {
      nearby_letter = i;
      break;
    }
  }
 
#elif
  bool new_position = uwb_parser_check_data();
#endif

  if (new_position) {
    Serial.println("New Position: " + String(current_position.x) + ", " + String(current_position.y));

    //if (prev_letter != nearby_letter) {
    if (nearby_letter != -1) {
      Serial.print("Nearby Position: ");
      //if (nearby_letter ==  -1) 
      //  Serial.println("None");
      //else
        Serial.println(letters[nearby_letter].letter);
    }
  }

  webserver_check_for_client();


}
