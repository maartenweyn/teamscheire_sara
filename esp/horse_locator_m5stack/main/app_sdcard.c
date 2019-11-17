#include "app_sdcard.h"
#include "main.h"
#include "app_config.h"

#include <stdio.h>
#include "esp_log.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "board_pins_config.h"

#include "periph_sdcard.h"
#include "tft.h"


#define TAG "APP_SDCARD:"

extern esp_periph_set_handle_t set;
static char debug_logfile[64];
static FILE* log_file;

#include "tft.h"

esp_err_t init_sdcard() {
  ESP_LOGI(TAG, "Initializing SD card");

#ifdef SDCARD_USE_SPI  

  ESP_LOGI(TAG, "Using SPI peripheral");

  disp_deselect();

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = SD_PIN_NUM_MISO;
  slot_config.gpio_mosi = SD_PIN_NUM_MOSI;
  slot_config.gpio_sck  = SD_PIN_NUM_CLK;
  slot_config.gpio_cs   = SD_PIN_NUM_CS;
  


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

  int retries = 0;

  sdmmc_card_t* card;

  while (retries < 10) {
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
    } else {
      // Card has been initialized, print its properties
      sdmmc_card_print_info(stdout, card);

      return ESP_OK;
    }
  }

#else
    // Initialize SD Card peripheral

  ESP_LOGI(TAG, "Using SD Card peripheral");
  periph_sdcard_cfg_t sdcard_cfg = {
      .root = "/sdcard",
      .card_detect_pin = get_sdcard_intr_gpio(), //GPIO_NUM_34
  };
  esp_periph_handle_t sdcard_handle = periph_sdcard_init(&sdcard_cfg);
  // Start sdcard & button peripheral
  esp_periph_start(set, sdcard_handle);

  // Wait until sdcard is mounted
  while (!periph_sdcard_is_mounted(sdcard_handle)) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
  }

#endif


  return ESP_FAIL;

  
}

esp_err_t deinit_sdcard() {
  return esp_vfs_fat_sdmmc_unmount();
}


esp_err_t init_debug_log() {
  if (app_config.store_ranges)
  {
    app_config.store_range_counter++;
    save_config();

    sprintf(debug_logfile, "/sdcard/%d.csv", app_config.store_range_counter);

    ESP_LOGI(TAG, "Opening file %s", debug_logfile);
    log_file = fopen(debug_logfile, "a+");
    if (log_file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
    }

    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t store_ranges(char* range_string) {
  static uint8_t counter = 0;
  if (log_file != NULL) {
    fprintf(log_file, "%s", range_string);
    ESP_LOGD(TAG, "writing to  %s: %s", debug_logfile, range_string);

    counter = (counter + 1) % 20;

    if (counter == 0) {
      fclose(log_file);
      ESP_LOGI(TAG, "flush file %s", debug_logfile);



      // /// DEBUG
      // FILE* f = fopen(debug_logfile, "r");
      // if (f == NULL) {
      //     ESP_LOGE(TAG, "Failed to open file for writing");
      // } else {
      //   char* read_buf=malloc(2048);
      //   memset(read_buf, 0, 2048);
      //   while(1){
      //     uint32_t r;
      //     r=fread(read_buf,1,2048,f);
      //     if(r>0){
      //       ESP_LOGI(TAG, "Content: %s", read_buf);
      //     }else
      //       break;
      //   }
      //   fclose(f);
      //   free(read_buf);
      // }


      // REOPEN FILE
      log_file = fopen(debug_logfile, "a+");
      if (log_file == NULL) {
          ESP_LOGE(TAG, "Failed to open file for writing");
      }
    }
    return ESP_OK;
  } else {
    return ESP_FAIL;
  }
}