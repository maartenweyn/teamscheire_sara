#include "uwb_parser.h"
#include "main.h"
#include "localization.h"

#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#define TAG "UWB_PARSER: "

#define BUF_SIZE (1024)

void uwb_parser_init() {
  uart_config_t uart_config = { .baud_rate = 500000, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits =
            UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
 
  uart_param_config(UART_NUM_2, &uart_config);
  uart_set_pin(UART_NUM_2, 23, 22, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
}

bool readLine(char* line) {
    int size;
    memset(line, 0 , BUF_SIZE);
    char *ptr = line;
    while (1) {
        size = uart_read_bytes(UART_NUM_2, (unsigned char *) ptr, 1, 1000/portMAX_DELAY);
        if (size == 1) {
            printf("%c" , ptr[0]);
            if (*ptr == '\n') {
                ptr++;
                *ptr = 0;
                return true;
            }
            ptr++;
        } else if (size == 0) {
          return false;
        } // End of read a character
    } // End of loop
} // End of readLine

// Parsing of UWB data
static bool setRange(int id, int range)
{
  static int last_id = 100;

  bool got_position = false;
  
  meas_ranges[id-1] = range / 10;
  for (int i = 0; i < 6; i++)
  {
    if ( i == id - 1) {
      meas_counter[i] = 0;
    } else {
      if (meas_counter[i] < 100)
        meas_counter[i]++;
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
  static char *data = (char *) malloc(BUF_SIZE);
  bool got_position = false;
  
  if (readLine(data)) {
    ESP_LOGI(TAG, "LINE: %s", data);
  } else {
    ESP_LOGI(TAG, "NO LINE: %s", data);
  }
  // if (myBuffer.readFromSerial(&Serial2, 30000)) {
  //   if (cmdParser.parseCmd(&myBuffer) != CMDPARSER_ERROR) {
  //     if (cmdParser.getParamCount() > 1 ) {
  //       counter = 0;
  //       got_position = setRange(atoi(cmdParser.getCommand()), atoi(cmdParser.getCmdParam(2)));
  //     } else {
  //       counter++;
  //       if (counter > 100)
  //         Serial.println(cmdParser.getCommand());
  //     }
  //   }
  // }

  return got_position;
 }

 void locator_task( void *pvParameters ){
   	(void) pvParameters;

     while(1) {
       uwb_parser_check_data();
     }

 }