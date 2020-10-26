
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_event.h"
#include "esp_log.h"
#include <sys/socket.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/dac.h"
#include <dirent.h>
#include "esp_heap_caps.h"
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"


#include "localization.h"
#include "uwb_parser.h"
#include "app_ble.h"

#include "app_config.h"
#define TAG "main:"


// app_config_t app_config = {
//   .sensor_height = DEF_SENSOR_HEIGHT,
//   .particle_filter = {
//     .nr_of_particles = DEF_NR_OF_PARTICLES,
//     .max_speed = DEF_MAX_SPEED,
//     .UWB_std2 = DEF_UWB_STD2
//   },
//   .field_size = {DEF_FIELD_SIZE_X, DEF_FIELD_SIZE_Y, 300},
//   .node_positions = {{0, 0, 100}, {2000, 0, 100},{2000, 4000, 100},{0, 4000, 100},{2000, 2000, 100},{0, 2000, 100}}
// };





void app_main()
{

  esp_err_t err = nvs_flash_init();
  ESP_LOGI(TAG, "nvs_flash_init %x", err);
  if (err == ESP_ERR_NVS_NO_FREE_PAGES  || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
      ESP_LOGI(TAG, "nvs_flash_init %x", err);
  }

  err = load_config();
  if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) reading data from NVS!\n", esp_err_to_name(err));

  err = start_bluetooth();
  if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) loading ble!\n", esp_err_to_name(err));

  //initialize_localization_engine();

  //uwb_parser_init();   



  //xTaskCreate(locator_task, "locator_task", 4096, NULL, 5, NULL);
  vTaskSuspend(NULL);

  //never goto here
  while(1){
      ESP_LOGI(TAG, "should not be here");
  }
}

