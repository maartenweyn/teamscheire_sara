
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
#include "aplay.h"


//#include "web_radio.h"


#define TAG "main:"
// typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
// typedef int (*http_cb) (http_parser*);


//char* http_body;

#define GPIO_OUTPUT_IO_0    5
#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_IO_0))

// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13




void test_task1(void* pvParameters) {
    while(1) {
      ESP_LOGI(TAG, "test_task1");
      //esp_task_wdt_feed();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void test_task2(void* pvParameters) {
    while(1) {
      ESP_LOGI(TAG, "test_task2");
      aplay_wav("/sdcard/001K.wav");
      //esp_light_sleep_start();
    }
}

esp_err_t init_sdcard() {
  ESP_LOGI(TAG, "Initializing SD card");
  ESP_LOGI(TAG, "Using SPI peripheral");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = PIN_NUM_MISO;
  slot_config.gpio_mosi = PIN_NUM_MOSI;
  slot_config.gpio_sck  = PIN_NUM_CLK;
  slot_config.gpio_cs   = PIN_NUM_CS;


  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
  // Please check its source code and implement error recovery when developing
  // production applications.
  sdmmc_card_t* card;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
          ESP_LOGE(TAG, "Failed to mount filesystem. "
              "If you want the card to be formatted, set format_if_mount_failed = true.");
      } else {
          ESP_LOGE(TAG, "Failed to initialize the card (%s). "
              "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
      }
      return ret;
  }

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card);

  // Use POSIX and C standard library functions to work with files.
  // First create a file.
  ESP_LOGI(TAG, "Opening file");
  FILE* f = fopen("/sdcard/hello.txt", "w");
  if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file for writing");
      return ESP_FAIL;
  }
  fprintf(f, "Hello %s!\n", card->cid.name);
  fclose(f);
  ESP_LOGI(TAG, "File written");

  return ESP_OK;
}

void app_main()
{
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

    init_sdcard();



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
   
   
    xTaskCreate(test_task1, "test_task1", 4096, NULL, 5, NULL);
    xTaskCreate(test_task2, "test_task2", 4096, NULL, 5, NULL);
    vTaskSuspend(NULL);

    //never goto here
    while(1){
        
    }
}

