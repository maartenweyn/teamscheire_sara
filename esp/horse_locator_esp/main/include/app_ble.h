#ifndef APP_BLE_H
#define APP_BLE_H

#include "esp_err.h"
#include "main.h"

#include "localization.h"

#define DEVICE_NAME "HORSE_LOCATOR"

#define DATA_BUFFER_LENGTH  512

/* Attributes State Machine */

enum
{
    IDX_SVC_POS,
    
    IDX_CHAR_POS,
    IDX_CHAR_VAL_POS,
    IDX_CHAR_CFG_POS,

    IDX_CHAR_POS_STR,
    IDX_CHAR_VAL_POS_STR,
    IDX_CHAR_CFG_POS_STR,

    // IDX_SVC_CONF,
    
    IDX_CHAR_CONF_NODES_POS,
    IDX_CHAR_VAL_CONF_NODES_POS,

    // IDX_SVC_DEBUG,
    
    IDX_CHAR_CONF_DEBUG_RANGES,
    IDX_CHAR_VAL_DEBUG_RANGES,
    IDX_CHAR_CFG_DEBUG_RANGES,

    IDX_CHAR_CONF_DEBUG_PARTICLES,
    IDX_CHAR_VAL_DEBUG_PARTICLES,
    IDX_CHAR_CFG_DEBUG_PARTICLES,

    IDX_NB,
};

typedef void (*new_orientation_cb_t)(int16_t omega, int16_t phi, int16_t psi, int16_t xm, int16_t ym, int16_t zm);

esp_err_t start_bluetooth();
void set_new_orientation_cb(new_orientation_cb_t cb);
esp_err_t ble_set_new_location();
esp_err_t set_new_ranges(int ranges[6]);
esp_err_t ble_push_particles(particle_t* particles, int length);

#endif