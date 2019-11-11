#include "uwb_parser.h"
#include "main.h"
#include "localization.h"

#include "driver/uart.h"
#include <string.h>

//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "board_pins_config.h"

#define TAG "UWB_PARSER: "

#define BUF_SIZE 1024
static char data[BUF_SIZE];

void uwb_parser_init() {
  uart_config_t uart_config = { 
    //.baud_rate = 500000, 
    .baud_rate = 115200, 
    .data_bits = UART_DATA_8_BITS, 
    .parity = UART_PARITY_DISABLE, 
    .stop_bits = UART_STOP_BITS_1, 
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 0,
    .use_ref_tick = 0
    };
 
  uart_param_config(UART_NUM_2, &uart_config);
  uart_set_pin(UART_NUM_2, UWB_UART_TX, UWB_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  esp_err_t res = uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
  if (res != 0)
    ESP_LOGE(TAG, "uart_driver_install error: %d:", res);
  else

    ESP_LOGI(TAG, "uart_driver_install ok");
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
  ESP_LOGD(TAG, "setRange: %d: %d mm", id, range);
  static int last_id = 100;
  //static int iteration = 0;

  bool got_position = false;
  
  //if (meas_counter[id-1] == 0)  
    meas_ranges[id-1] = range / 10;
  //else 
  //  meas_ranges[id-1] += (range / 10);

  //meas_counter[id-1]++;

  for (int i = 0; i < 6; i++)
  {
    if ( i == id - 1) {
      meas_absence_counter[i] = 0;
      receiving_ranges = true;
    } else {
      if (meas_absence_counter[i] < 100)
        meas_absence_counter[i]++;
    }
  }

  if (id <= last_id) { 
    //iteration++;
    // if (iteration >= MEAS_AVERAGE)
    // {
    //   iteration = 0;
    //   for (int i = 0; i < 6; i++) {
    //     if (meas_counter[i] > 1)
    //     {
    //       ESP_LOGD(TAG, "meas_ranges: %d: %d cm (%d)", i+1, meas_ranges[i], meas_counter[i]);
    //       avg_meas_ranges[i] = meas_ranges[i] / meas_counter[i];
    //       ESP_LOGD(TAG, " --> %d", meas_ranges[i]);
    //     }
    //     meas_counter[i] = 0;

    //   }

      got_position = processMeasurement(); 
    //} 
  }

  last_id = id;

  return got_position;
}

// void uwb_test_range() {

//   bool got_position =  false;

//   //[-1, -1, 20.84, 0.76, -1, -1]
//   // got_position = setRange(3, 20840);
//   // got_position = setRange(4, 760);
//   // got_position = setRange(3, 20840);

//   //[1345, 2479, 5006, 4694, 2429, -1] # r

//   // setRange(1, 13450);
//   // got_position = setRange(2, 24790);
//   // got_position = setRange(3, 50060);
//   // got_position = setRange(4, 46940);
//   // got_position = setRange(5, 24290);
//   // got_position = setRange(1, 13450);

//   //#ranges = [5.32, 20.87, -1, -1, 29.47, -1] # m

//   // setRange(1, 5320);
//   // got_position = setRange(2, 20870);
//   // meas_counter[3-1] = 100;
//   // meas_counter[4-1] = 100;
//   // got_position = setRange(5, 29470);
//   // got_position = setRange(1, 5320);

//   //#ranges = [9.02, 11.60, 60.38, 60.89, 29.38] # c
//   setRange(1, 9020);
//   got_position = setRange(2, 11600);
//   got_position = setRange(3, 60380);
//   got_position = setRange(4, 60380);
//   got_position = setRange(5, 29380);
//   got_position = setRange(1, 9020);


//   //#ranges = [28.52, -1, 34.39, 34.39, 11.39] # x
//   setRange(1, 28250);
//   meas_counter[2-1] = 100;
//   got_position = setRange(3, 34390);
//   got_position = setRange(4, 34390);
//   got_position = setRange(5, 11390);
//   got_position = setRange(1, 28250);
// }

 bool uwb_parser_check_data() {
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


    //ESP_LOGI(TAG, "LINE: (%d) %s", strlen(data), data);

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
