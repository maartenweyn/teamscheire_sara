#ifndef APP_SDCARD_H
#define APP_SDCARD_H


#include "esp_err.h"
esp_err_t init_sdcard();
esp_err_t deinit_sdcard();
esp_err_t init_debug_log();
esp_err_t store_ranges(char* range_string);

#endif