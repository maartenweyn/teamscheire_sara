/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "wm8978.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include <sys/socket.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "eth.h"
#include "event.h"
#include "wifi.h"
#include "hal_i2c.h"
#include "hal_i2s.h"
#include "wm8978.h"
#include "webserver.h"
#include "http.h"
#include "cJSON.h"
#include "mdns_task.h"
#include "audio.h"
#include <dirent.h>
#include "esp_heap_caps.h"
#include "euler.h"
#include "websocket.h"
#include "esp_heap_caps.h"


//#include "web_radio.h"


#define TAG "main:"
// typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
// typedef int (*http_cb) (http_parser*);


//char* http_body;

#define GPIO_OUTPUT_IO_0    5
#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_IO_0))




//16-bit 128-sample sine wavetable
int16_t sineT[128]= {0, 1620, 3237, 4845, 6442, 8023, 9584, 11122, 
12633, 14113, 15558, 16965, 18331, 19651, 20924, 22146, 
23313, 24423, 25474, 26462, 27385, 28241, 29029, 29745, 
30388, 30957, 31451, 31867, 32205, 32465, 32645, 32745, 
32765, 32705, 32565, 32345, 32046, 31668, 31213, 30682, 
30076, 29396, 28644, 27822, 26931, 25975, 24956, 23875, 
22736, 21541, 20294, 18997, 17653, 16266, 14840, 13377, 
11881, 10356, 8806, 7234, 5645, 4042, 2429, 810, -810, 
-2429, -4042, -5646, -7235, -8807, -10357, -11881, -13377, 
-14840, -16267, -17653, -18997, -20294, -21542, -22736, 
-23875, -24956, -25976, -26932, -27822, -28644, -29396, 
-30076, -30682, -31214, -31668, -32046, -32345, -32565, -32705, 
-32765, -32745, -32645, -32465, -32205, -31867, -31450, -30957, 
-30388, -29745, -29028, -28241, -27385, -26461, -25473, -24423, 
-23313, -22145, -20924, -19651, -18330, -16965, -15557, -14112, 
-12632, -11122, -9584, -8022, -6441, -4845, -3236, -1620};


uint32_t missedByteCounter = 0; //debug variable

static uint16_t setup_triangle_sine_waves(int bits)
{
  static uint8_t index = 0;
  uint16_t samples_data = sineT[index/2] + 32765;
   //write to codec below
  int bytes_written = hal_i2s_write(0, (const char *)&samples_data, 2, 100);
   
  if (bytes_written > 0)  index++; 
  else missedByteCounter++;

  index+=11;
 // missedByteCounter+=bytes_written;
   return samples_data;
}

void i2sLoop(){
  setup_triangle_sine_waves(0);
}


void test_task(void* pvParameters) {
    while(1) {
        ESP_LOGI(TAG, "test_task");
        i2sLoop();
    }
    
}

void app_main()
{
    esp_err_t err;
    event_engine_init();
    nvs_flash_init();
    tcpip_adapter_init();
    wifi_init_softap("teamscheire","123456789");
    /*init gpio*/
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_IO_0, 0);
    /*init codec */
    hal_i2c_init(0,19,18);
    hal_i2s_init(0,48000,16,2);
    WM8978_Init();
    WM8978_ADDA_Cfg(1,1); 
    WM8978_Input_Cfg(1,0,0);     
    WM8978_Output_Cfg(1,0); 
    WM8978_MIC_Gain(0);
    WM8978_AUX_Gain(0);
    WM8978_LINEIN_Gain(0);
    WM8978_SPKvol_Set(35);
    WM8978_HPvol_Set(35,35); //0-63
    WM8978_EQ_3D_Dir(0);
    WM8978_EQ1_Set(0,24);
    WM8978_EQ2_Set(0,24);
    WM8978_EQ3_Set(0,24);
    WM8978_EQ4_Set(0,24);
    WM8978_EQ5_Set(0,24);



    // xEventGroupWaitBits(station_event_group,STA_GOTIP_BIT,pdTRUE,pdTRUE,portMAX_DELAY);
    // //print ip address
    // tcpip_adapter_ip_info_t ip;
    // memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
    // if (tcpip_adapter_get_ip_info(ESP_IF_WIFI_AP, &ip) == 0) {
    //     ESP_LOGI(TAG, "~~~~~~~~~~~");
    //     ESP_LOGI(TAG, "ETHIP:"IPSTR, IP2STR(&ip.ip));
    //     ESP_LOGI(TAG, "ETHPMASK:"IPSTR, IP2STR(&ip.netmask));
    //     ESP_LOGI(TAG, "ETHPGW:"IPSTR, IP2STR(&ip.gw));
    //     ESP_LOGI(TAG, "~~~~~~~~~~~");
    // }
    // /*print the last ram*/
    // size_t free8start=heap_caps_get_free_size(MALLOC_CAP_8BIT);
    // size_t free32start=heap_caps_get_free_size(MALLOC_CAP_32BIT);
    // ESP_LOGI(TAG,"free mem8bit: %d mem32bit: %d\n",free8start,free32start);
   
   
    xTaskCreate(test_task, "test_task", 4096, NULL, 5, NULL);
    vTaskSuspend(NULL);

    //never goto here
    while(1){
        i2sLoop();
    }
}

