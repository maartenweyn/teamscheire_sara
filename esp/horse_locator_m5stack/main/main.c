
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include <sys/socket.h>
#include "nvs.h"
#include "nvs_flash.h"
//#include "eth.h"
///#include "event.h"
// #include "wifi.h"
//#include "hal_i2c.h"
//#include "hal_i2s.h"
//#include "wm8978.h"
#include "webserver.h"
// #include "http.h"
#include "cJSON.h"
// #include "mdns_task.h"
// #include "audio.h"
#include <dirent.h>
#include "esp_heap_caps.h"
// #include "euler.h"
// #include "websocket.h"
#include "esp_heap_caps.h"
// #include "aplay.h"
#include "esp_task_wdt.h"

#include "board.h"

#include "periph_sdcard.h"

#include "app_sound.h"
#include "app_config.h"
#include "app_sdcard.h"
#include "localization.h"
#include "uwb_parser.h"
#include "app_leds.h"

 #include <http_server.h>


//#include "web_radio.h"


#define TAG "main:"
// typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
// typedef int (*http_cb) (http_parser*);


//char* http_body;

//#define GPIO_OUTPUT_IO_0    5
//#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_IO_0))

static esp_periph_set_handle_t set;



void app_main()
{
  //event_engine_init();
  esp_err_t err = nvs_flash_init()
  ;ESP_LOGI(TAG, "nvs_flash_init %x", err);
  if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
      ESP_LOGI(TAG, "nvs_flash_init %x", err);
  }

  

  

  esp_periph_config_t periph_cfg = DEFAULT_ESP_PHERIPH_SET_CONFIG();
  set = esp_periph_set_init(&periph_cfg);

  ESP_LOGI(TAG, "[ 1 ] Mount sdcard");
  ESP_LOGI(TAG, "[4.1] Start and wait for SDCARD to mount");

  #ifndef SKIP_SD_CARD
  init_sdcard();
  #endif

  load_config();

  setup_player();


  //play_letter('a');

  leds_init();
  leds_blink(0, 255, 0, 0, 500);


  static httpd_handle_t server = NULL;
  initialise_wifi(&server);

  // uwb_parser_init();   


  xTaskCreate(locator_task, "locator_task", 4096, NULL, 5, NULL);
  // //xTaskCreatePinnedToCore(locator_task, "locator_task", 4096, NULL, 5, NULL, 1);
  // //xTaskCreate(webserver_task, "webserver_task", 4096, NULL, 10, NULL);
  // //xTaskCreatePinnedToCore(webserver_task, "webserver_task", 4096, NULL, 10, NULL, 0);
  vTaskSuspend(NULL);

  //never goto here
  while(1){
      
  }
}

