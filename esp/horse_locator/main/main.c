
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "wm8978.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
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
#include "aplay.h"
#include "esp_task_wdt.h"

#include "app_sound.h"
#include "app_config.h"
#include "app_sdcard.h"
#include "localization.h"
#include "uwb_parser.h"
#include "app_leds.h"


//#include "web_radio.h"


#define TAG "main:"
// typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
// typedef int (*http_cb) (http_parser*);


//char* http_body;

#define GPIO_OUTPUT_IO_0    5
#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_IO_0))


void app_main()
{
  event_engine_init();
  nvs_flash_init();

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
  hal_i2s_init(0,41000,16,2);
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

  init_sdcard();
  load_config();

  // Access point
  tcpip_adapter_init();
  wifi_init_softap(app_config.wifi_ssid,app_config.wifi_passwd);    

  tcpip_adapter_ip_info_t ip;
  tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip);    
  ESP_LOGI(TAG, "SoftAP IP=%s", inet_ntoa(ip.ip.addr));

  //leds_init();
  uwb_parser_init();   


  //xTaskCreate(locator_task, "locator_task", 4096, NULL, 5, NULL);
  xTaskCreatePinnedToCore(locator_task, "locator_task", 4096, NULL, 5, NULL, 1);
  //xTaskCreate(webserver_task, "webserver_task", 4096, NULL, 10, NULL);
  xTaskCreatePinnedToCore(webserver_task, "webserver_task", 4096, NULL, 10, NULL, 0);
  vTaskSuspend(NULL);

  //never goto here
  while(1){
      
  }
}

