#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "main.h"
#include "esp_err.h"

#define DEF_NEARBY_THRESHOLD 300 
#define DEF_FIELD_SIZE_X      2000
#define DEF_FIELD_SIZE_Y      6000
#define DEF_FIELD_SIZE_MARGIN 400

typedef struct {
 char wifi_ssid[20];
 char wifi_passwd[20];
 position_t node_positions[6];
 int nearby_threshold;
 position_t field_size;
 int field_size_margin;
 letter_position_t letters[NR_OF_LETTERS];
} app_config_t;

extern app_config_t app_config;

void create_default_config();
esp_err_t save_config();
esp_err_t load_config();

#endif