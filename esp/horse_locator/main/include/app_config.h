#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "main.h"
#include "esp_err.h"

#define DEF_NEARBY_THRESHOLD 300 

typedef struct {
 char wifi_ssid[20];
 char wifi_passwd[20];
 position_t node_positions[6];
 int nearby_threshold;
} app_config_t;

extern app_config_t app_config;

void create_default_config();
esp_err_t save_config();
esp_err_t load_config();

#endif