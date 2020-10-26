#ifndef APP_BLE_H
#define APP_BLE_H

#include "esp_err.h"
#include "main.h"

#define DEVICE_NAME "HORSE_LOCATOR"

#define DATA_BUFFER_LENGTH  512

/* Attributes State Machine */

enum
{
    IDX_SVC,
    
    IDX_CHAR_POS,
    IDX_CHAR_VAL_POS,
    IDX_CHAR_CFG_POS,

    IDX_CHAR_POS_STR,
    IDX_CHAR_VAL_POS_STR,
    IDX_CHAR_CFG_POS_STR,

    // IDX_CHAR_B,
    // IDX_CHAR_VAL_B,

    // IDX_CHAR_C,
    // IDX_CHAR_VAL_C,
    // IDX_CHAR_CFG_C,

    IDX_NB,
};

typedef void (*new_orientation_cb_t)(int16_t omega, int16_t phi, int16_t psi, int16_t xm, int16_t ym, int16_t zm);

esp_err_t start_bluetooth();
void set_new_orientation_cb(new_orientation_cb_t cb);
esp_err_t set_new_location();

#endif