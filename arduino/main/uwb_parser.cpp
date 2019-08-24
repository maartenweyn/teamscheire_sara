#include "uwb_parser.h"
#include "main.h"
#include "localization.h"

#include <CmdBuffer.hpp>
#include <CmdCallback.hpp>
#include <CmdParser.hpp>

CmdParser cmdParser;
CmdCallback<3> cmdCallback;
CmdBuffer<32> myBuffer;

void uwb_parser_init() {
  Serial2.begin(500000);  // UART of UWB module

  cmdParser.setOptSeperator(',');

}

// Parsing of UWB data
static bool setRange(int id, int range)
{
  static int last_id = 100;

  bool got_position = false;
  
  ranges[id-1] = range / 10;
  for (int i = 0; i < 6; i++)
  {
    if ( i == id - 1) {
      counter[i] = 0;
    } else {
      if (counter[i] < 100)
        counter[i]++;
    }
  }

  if (id < last_id) { 
    got_position = processMeasurement();
  }

  last_id = id;

  return got_position;
}


 bool uwb_parser_check_data() {
  static int counter = 0;

  bool got_position = false;
  
  if (myBuffer.readFromSerial(&Serial2, 30000)) {
    if (cmdParser.parseCmd(&myBuffer) != CMDPARSER_ERROR) {
      if (cmdParser.getParamCount() > 1 ) {
        counter = 0;
        got_position = setRange(atoi(cmdParser.getCommand()), atoi(cmdParser.getCmdParam(2)));
      } else {
        counter++;
        if (counter > 100)
          Serial.println(cmdParser.getCommand());
      }
    }
  }

  return got_position;
 }
