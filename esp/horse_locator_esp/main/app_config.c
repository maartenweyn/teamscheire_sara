#include "app_config.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

const position_t default_node_positions[6] = {{0, 0, 100}, {2000, 0, 100},{2000, 4000, 100},{0, 4000, 100},{2000, 2000, 100},{0, 2000, 100}};

#define STORAGE_NAMESPACE "storage"
#define TAG "APP_CONFIG:"

app_config_t app_config;

void create_default_config() {
  memcpy(app_config.node_positions, default_node_positions, sizeof(default_node_positions));
  app_config.nearby_threshold = DEF_NEARBY_THRESHOLD;
  app_config.sensor_height = DEF_SENSOR_HEIGHT;
  app_config.field_size.x = DEF_FIELD_SIZE_X;
  app_config.field_size.y = DEF_FIELD_SIZE_Y;
  app_config.field_size_margin = DEF_FIELD_SIZE_MARGIN;
  app_config.volume = DEF_VOLUME;
  app_config.particle_filter.nr_of_particles = DEF_NR_OF_PARTICLES;
  app_config.particle_filter.max_speed = DEF_MAX_SPEED;
  app_config.particle_filter.UWB_std2 = DEF_UWB_STD2;
  app_config.particle_filter.std_min_threshold = DEF_STD_THRESH;
  app_config.store_ranges = 0;
  app_config.store_range_counter = 0;
}

/* Read config from NVS
   Return an error if anything goes wrong
   during this process.
 */
esp_err_t load_config(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Read run time blob
    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    // obtain required memory space to store blob being read from NVS
    err = nvs_get_blob(my_handle, "app_config", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    if (required_size == 0) {
        ESP_LOGE(TAG, "Nothing saved yet! - create default config\n");
        nvs_close(my_handle);

        create_default_config();
        return save_config();
    } else {
        //app_config_t* app_config = malloc(required_size);
        if (required_size == sizeof(app_config_t)) {
          err = nvs_get_blob(my_handle, "app_config", (void*) &app_config, &required_size);
          if (err != ESP_OK) {
              return err;
          }   
          ESP_LOGI(TAG, "Loaded Config\n");
      } else {
        // get default values
        ESP_LOGE(TAG, "Size does not match!- create default config\n");
        nvs_close(my_handle);

        create_default_config();
        return save_config();
      }
    }

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}


esp_err_t save_config() {
  nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Write
    err = nvs_set_blob(my_handle, "app_config", (void*) &app_config, sizeof(app_config_t));
    if (err != ESP_OK) return err;

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

