
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "driver/rmt.h"
#include "driver/uart.h"            // for the uart driver access
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
#include "led_strip.h"
#include "dfplayer.h"

#include "app_config.h"
#define TAG "main:"

static led_strip_t *strip;

void set_leds_color(int red, int green, int blue)
{
  for (int i = 0; i < NR_LEDS; i++)
    ESP_ERROR_CHECK(strip->set_pixel(strip, i, red, green, blue));

  ESP_ERROR_CHECK(strip->refresh(strip, 100));
}

void app_main()
{
  //LEDSTRIP
  rmt_config_t config = RMT_DEFAULT_CONFIG_TX(NEOPIXEL_PIN, NEOPIXEL_RMT_CHANNEL);
  // set counter clock to 40MHz
  config.clk_div = 2;

  ESP_ERROR_CHECK(rmt_config(&config));
  ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

  led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(NR_LEDS, (led_strip_dev_t)config.channel);
  strip = led_strip_new_rmt_ws2812(&strip_config);
  if (!strip) {
      ESP_LOGE(TAG, "install WS2812 driver failed");
  }

  set_leds_color(0, 0, 255);
  ESP_LOGI(TAG, "leds init ok");



  // FLASH
  esp_err_t err = nvs_flash_init();
  ESP_LOGI(TAG, "nvs_flash_init %x", err);
  if (err == ESP_ERR_NVS_NO_FREE_PAGES  || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
      ESP_LOGI(TAG, "nvs_flash_init %x", err);
  }

  // CONFIG
  err = load_config();
  if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) reading data from NVS!\n", esp_err_to_name(err));


  // SOUND
  // Set UART pins(TX: IO16 (UART2 default), RX: IO17 (UART2 default)
  dfplayer_init(UART_NUM_2, true, true);
  

  // BLE
  err = start_bluetooth();
  if (err != ESP_OK) ESP_LOGE(TAG, "Error (%s) loading ble!\n", esp_err_to_name(err));

  // LOCALIZATION
  //initialize_localization_engine();

  //uwb_parser_init();   



  //xTaskCreate(locator_task, "locator_task", 4096, NULL, 5, NULL);

   set_leds_color(0, 0, 0);

  vTaskSuspend(NULL);

  //never goto here
  while(1){
      ESP_LOGI(TAG, "should not be here");
  }
}

