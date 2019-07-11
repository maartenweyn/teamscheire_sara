#include <CmdBuffer.hpp>
#include <CmdCallback.hpp>
#include <CmdParser.hpp>

#define LED 2

CmdParser cmdParser;
CmdCallback<3> cmdCallback;
CmdBuffer<32> myBuffer;

int ranges[6] = {-1,-1,-1,-1,-1,-1};
int counter[6] = {-1,-1,-1,-1,-1,-1};

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  Serial2.begin(500000);

  //cmdCallback.addCmd(strError, &funcError);
  cmdParser.setOptSeperator(',');
  
  //cmdCallback.addCmd("1", &node1);
  //cmdCallback.addCmd("2", &node2);
}

// the loop function runs over and over again forever
void loop() {


  // Auto Handling
  //cmdCallback.loopCmdProcessing(&myParser, &myBuffer, &Serial2);

  if (myBuffer.readFromSerial(&Serial2, 30000)) {
    if (cmdParser.parseCmd(&myBuffer) != CMDPARSER_ERROR) {
            //Serial.print("Line have readed: ");
            //Serial.println(myBuffer.getStringFromBuffer());


            if (cmdParser.getParamCount() > 1 ) {

              setRange(atoi(cmdParser.getCommand()), atoi(cmdParser.getCmdParam(2)));

            //Serial.print("Command: ");
            //Serial.println(cmdParser.getCommand());

            //Serial.print("Size of parameter: ");
            //Serial.println(cmdParser.getParamCount());

//            const size_t count = cmdParser.getParamCount();
//            for (size_t i = 0; i < count; i++) {
//
//                Serial.print("Param ");
//                Serial.print(i);
//                Serial.print(": ");
//                Serial.println(cmdParser.getCmdParam(i));
//            }
            }
        } else {
            Serial.println("Parser error!");
        }
  }

}

void setRange(int id, int range)
{
 

  ranges[id-1] = range;
  for (int i = 0; i < 6; i++)
  {
    if ( i == id - 1) 
      counter[i] = 0;
    else 
      counter[i]++;
  }

  for (int i = 0; i < 6; i++)
  {
    Serial.print(ranges[i]);
    Serial.print("\t");
  }

  Serial.println();
  
  
}
