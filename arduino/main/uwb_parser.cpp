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
static void setRange(int id, int range)
{
  static int last_id = 100;
  ranges[id-1] = range;
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
    processMeasurement();
  }

  last_id = id;
}


 void uwb_parser_check_data() {
    if (myBuffer.readFromSerial(&Serial2, 30000)) {
      if (cmdParser.parseCmd(&myBuffer) != CMDPARSER_ERROR) {
        if (cmdParser.getParamCount() > 1 ) {
              setRange(atoi(cmdParser.getCommand()), atoi(cmdParser.getCmdParam(2)));
        }
      }
    }
 }
