#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "main.h"
#include "esp_err.h"

#define DEF_NEARBY_THRESHOLD  300 
#define DEF_FIELD_SIZE_X      2000
#define DEF_FIELD_SIZE_Y      6000
#define DEF_FIELD_SIZE_MARGIN 400
#define DEF_NR_OF_PARTICLES   500
#define DEF_MAX_SPEED         200
#define DEF_UWB_STD2          2500
#define DEF_STD_THRESH        400
#define DEF_SENSOR_HEIGHT     250
#define DEF_NODE_HEIGHT       100
#define DEF_VOLUME            50
#define DEF_SSID              "teamscheire"
#define DEF_passwd            "0123456789"

typedef struct {
  int nr_of_particles;
  int max_speed;
  int UWB_std2;
  int std_min_threshold;
} particle_filter_config_t;

typedef struct {
 char wifi_ssid[20];
 char wifi_passwd[20];
 position_t node_positions[6];
 int nearby_threshold;
 int sensor_height;
 position_t field_size;
 int field_size_margin;
 uint8_t volume;
 letter_position_t letters[NR_OF_LETTERS];
 particle_filter_config_t particle_filter;
 uint8_t store_ranges;
 uint8_t store_range_counter;
} app_config_t;

extern app_config_t app_config;

void create_default_config();
esp_err_t save_config();
esp_err_t load_config();

#endif