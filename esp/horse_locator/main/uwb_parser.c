#include "uwb_parser.h"
#include "main.h"
#include "localization.h"

#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#define TAG "UWB_PARSER: "

#define BUF_SIZE 1024
static char data[BUF_SIZE];

void uwb_parser_init() {
  uart_config_t uart_config = { 
    .baud_rate = 500000, 
    .data_bits = UART_DATA_8_BITS, 
    .parity = UART_PARITY_DISABLE, 
    .stop_bits = UART_STOP_BITS_1, 
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 0,
    .use_ref_tick = 0
    };
 
  uart_param_config(UART_NUM_2, &uart_config);
  uart_set_pin(UART_NUM_2, 22, 23, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void readLine(char* line) {
    int size;
    memset(line, 0 , BUF_SIZE);
    char *ptr = line;
    while (1) {
        size = uart_read_bytes(UART_NUM_2, (unsigned char *) ptr, 1, 1000/portMAX_DELAY);
        if (size == 1) {
            //printf("%c" , ptr[0]);
            if (*ptr == '\n') {
                ptr++;
                *ptr = 0;
                return;
            }
            ptr++;
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


 int uwb_parser_check_data() {
  static int counter = 0;
  bool got_position = false;
  
  readLine(data); //blocking
  //ESP_LOGI(TAG, "LINE: (%d) %s", strlen(data), data);

  char* pch;
  pch=strchr(data,',');
  if ((pch == NULL) || (pch-data != 1)) {
    counter++;
    if (counter > 100) {
      ESP_LOGI(TAG, "ERROR: (%d) %s", strlen(data), data);
      counter = 0;
    }
  } else {
    counter = 0;
    //LINE: (86) CC2538: ID: 0xb964, rev.: PG2.0, Flash: 512 KiB, SRAM: 32 KiB, AES/SHA: 1, ECC/RSA: 1
    //LINE: (24) 2,323947867395,21996519

    char* ptr;
    char delim[] = ",";


    ESP_LOGI(TAG, "LINE: (%d) %s", strlen(data), data);

    //range
    ptr=strtok(data,delim);
    int id = atoi(ptr);

    //timestamp
    ptr = strtok(NULL, delim);
    if (ptr == NULL) {
      counter++;
      return false;
    }

    //range
    ptr = strtok(NULL, delim);
    if (ptr == NULL) {
      counter++;
      return false;
    }

    int range = atoi(ptr);

    ptr = strtok(NULL, delim);
    if (ptr != NULL) {
      counter++;
      printf ("found unexpected %s\n",ptr);
      return false;
    }

    got_position = setRange(id, range);
  }

  return got_position;
 }

 void locator_task( void *pvParameters ){
   	(void) pvParameters;

     while(1) {
       uwb_parser_check_data();
     }

 }