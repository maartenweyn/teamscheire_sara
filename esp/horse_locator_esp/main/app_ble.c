#include "app_ble.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"

#define GATTS_TABLE_TAG "GATTS_TABLE"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SVC_INST_ID                 0

/* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
*  the data length must be less than GATTS_CHAR_VAL_LEN_MAX. 
*/
#define GATTS_CHAR_VAL_LEN_MAX      512
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

static uint8_t adv_config_done       = 0;
static bool is_connected = false;

uint16_t handle_table[IDX_NB];

new_orientation_cb_t new_orientation_cb = NULL;

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;

static uint8_t raw_adv_data[] = {
        // length, type, data
        /* flags */
        0x02, 0x01, 0x06, // FLAG: 110b : LE General Discoverable Mode, BR/EDR Not Supported.
        /* tx power*/
        0x02, 0x0a, 0xeb,
        /* service uuid */
        0x03, 0x03, 0xF0, 0xFF,// Survice UID 0xFFF0, FFF1 and FFF2
        /* device name */
        0x0d, 0x09, 'H', 'O', 'R', 'S', 'E', 'L', 'O', 'C', 'A', 'T', 'O','R'
};
static uint8_t raw_scan_rsp_data[] = {
        /* flags */
        0x02, 0x01, 0x06,
        /* tx power */
        0x02, 0x0a, 0xeb,
        /* service uuid */
        0x03, 0x03, 0xFF,0x00
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Service */
static const uint16_t GATTS_SERVICE_UUID_LOCALIZATION     = 0xFFF0;

//GATTS_SERVICE_UUID_LOCALIZATION Characteristics
static const uint16_t GATTS_CHAR_UUID_LOCALIZATION_POSTITION              = 0xF001;
static const uint16_t GATTS_CHAR_UUID_LOCALIZATION_POSTITION_STRING       = 0xF002;

//GATTS_SERVICE_UUID_CONFIGURATION Characteristics
static const uint16_t GATTS_CHAR_UUID_CONFIGURATION_NODE_POSITIONS        = 0xF101;

//GATTS_SERVICE_UUID__DEBUG Characteristics
static const uint16_t GATTS_CHAR_UUID_CONFIGURATION_DEBUG_RANGES            = 0xF201;
static const uint16_t GATTS_CHAR_UUID_CONFIGURATION_DEBUG_PARTICLES         = 0xF202;


static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read_write          =  ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_notify         =  ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
//static const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
//static const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t orientation_notifcation_ccc[2]      = {0x00, 0x00};
static char current_position_string[50] = "(0,0,0) - not initialzed";
static uint8_t uwb_ranges[6*4] = {0,};
static uint8_t ble_particles[50*10] = {0,};

app_config_t app_config;
//static uint8_t orientation_act_value[12]       = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// Orientation angles calculated from sensor
// static uint8_t orientation_sensor_value[6]    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static bool enable_location_act_ntf = false;
static bool enable_location_string_act_ntf = false;
static bool enable_orientation_sensor_ntf = false;
static bool enable_debug_ranges_act_ntf = false;
static bool enable_debug_particles_act_ntf = false;


/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[IDX_NB] =
{

    // Service Declaration : GATTS_SERVICE_UUID_LOCALIZATION
    [IDX_SVC_POS]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_LOCALIZATION), (uint8_t *)&GATTS_SERVICE_UUID_LOCALIZATION}},


    /* Characteristic Declaration  GATTS_CHAR_UUID_LOCALIZATION_POSTITION */
    [IDX_CHAR_POS]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_POS] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_LOCALIZATION_POSTITION, ESP_GATT_PERM_READ,
      GATTS_CHAR_VAL_LEN_MAX, sizeof(current_position), (uint8_t *)&current_position}},

    // Client Characteristic Configuration Notification Descriptor
    [IDX_CHAR_CFG_POS]           =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      sizeof(uint16_t),sizeof(orientation_notifcation_ccc), (uint8_t *)orientation_notifcation_ccc}},

    /* Characteristic Declaration  GATTS_CHAR_UUID_LOCALIZATION_POSTITION_STRING*/
    [IDX_CHAR_POS_STR]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_POS_STR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_LOCALIZATION_POSTITION_STRING, ESP_GATT_PERM_READ,
      GATTS_CHAR_VAL_LEN_MAX, sizeof(current_position_string), (uint8_t *)current_position_string}},

    // Client Characteristic Configuration Notification Descriptor
    [IDX_CHAR_CFG_POS_STR]           =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      sizeof(uint16_t),sizeof(orientation_notifcation_ccc), (uint8_t *)orientation_notifcation_ccc}},






       /* Characteristic Declaration  GATTS_CHAR_UUID_LOCALIZATION_POSTITION */
    [IDX_CHAR_CONF_NODES_POS]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_CONF_NODES_POS] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_CONFIGURATION_NODE_POSITIONS, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      GATTS_CHAR_VAL_LEN_MAX, sizeof(app_config.node_positions), (uint8_t *)app_config.node_positions}},





       /* Characteristic Declaration  GATTS_CHAR_UUID_CONFIGURATION_DEBUG_RANGES */
    [IDX_CHAR_CONF_DEBUG_RANGES]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_DEBUG_RANGES] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_CONFIGURATION_DEBUG_RANGES, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      GATTS_CHAR_VAL_LEN_MAX, sizeof(uwb_ranges), (uint8_t *)uwb_ranges}},

    // Client Characteristic Configuration Notification Descriptor
    [IDX_CHAR_CFG_DEBUG_RANGES]           =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      sizeof(uint16_t),sizeof(orientation_notifcation_ccc), (uint8_t *)orientation_notifcation_ccc}},




        [IDX_CHAR_CONF_DEBUG_PARTICLES]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_DEBUG_PARTICLES] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_CONFIGURATION_DEBUG_PARTICLES, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      GATTS_CHAR_VAL_LEN_MAX, 500, ble_particles}},



    // Client Characteristic Configuration Notification Descriptor
    [IDX_CHAR_CFG_DEBUG_PARTICLES]           =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
      sizeof(uint16_t),sizeof(orientation_notifcation_ccc), (uint8_t *)orientation_notifcation_ccc}},

};

//static void new_orientation(uint8_t* raw);
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "advertising start failed");
            }else{
                ESP_LOGI(GATTS_TABLE_TAG, "advertising start successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "Stop adv successfully\n");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}

void example_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(GATTS_TABLE_TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;
    if (prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(GATTS_TABLE_TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    } else {
        if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(GATTS_TABLE_TAG, "Send response error");
            }
            free(gatt_rsp);
        }else{
            ESP_LOGE(GATTS_TABLE_TAG, "%s, malloc failed", __func__);
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
  ESP_LOGI(GATTS_TABLE_TAG, "example_exec_write_event_env")
;    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
        esp_log_buffer_hex(GATTS_TABLE_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(GATTS_TABLE_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
            if (set_dev_name_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }

            esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
            if (raw_adv_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
            if (raw_scan_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
 
            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, IDX_NB, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT, handle = %d", param->read.handle);
       	    break;
        case ESP_GATTS_WRITE_EVT:
            if (!param->write.is_prep){
                ESP_LOGI(GATTS_TABLE_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
                esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
                if (handle_table[IDX_CHAR_CFG_POS] == param->write.handle && param->write.len == 2){
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify enable");
                        enable_location_act_ntf = true;
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[IDX_CHAR_CFG_POS],
                                                sizeof(current_position), (uint8_t*)&current_position, false);
                    } 
                    else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify disable ");
                        enable_location_act_ntf = false;
                    }else{
                        ESP_LOGE(GATTS_TABLE_TAG, "unknown descr value");
                        esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
                    }
                } else if (handle_table[IDX_CHAR_CFG_POS_STR] == param->write.handle && param->write.len == 2){
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify enable");
                        enable_location_string_act_ntf = true;
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[IDX_CHAR_CFG_POS_STR],
                                                sizeof(current_position_string), (uint8_t*)current_position_string, false);
                    } 
                    else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify disable ");
                        enable_location_string_act_ntf = false;
                    }else{
                        ESP_LOGE(GATTS_TABLE_TAG, "unknown descr value");
                        esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
                    }
                } else if (handle_table[IDX_CHAR_CFG_DEBUG_RANGES] == param->write.handle && param->write.len == 2){
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify enable");
                        enable_debug_ranges_act_ntf = true;
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[IDX_CHAR_CFG_DEBUG_RANGES],
                                                sizeof(uwb_ranges), (uint8_t*)uwb_ranges, false);
                    } 
                    else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify disable ");
                        enable_debug_ranges_act_ntf = false;
                    }else{
                        ESP_LOGE(GATTS_TABLE_TAG, "unknown descr value");
                        esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
                    }
                } else if (handle_table[IDX_CHAR_CFG_DEBUG_PARTICLES] == param->write.handle && param->write.len == 2){
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify enable");
                        enable_debug_particles_act_ntf = true;
                        // esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[IDX_CHAR_CFG_DEBUG_PARTICLES],
                                                // sizeof(uwb_ranges), (uint8_t*)uwb_ranges, false);
                    } 
                    else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify disable ");
                        enable_debug_particles_act_ntf = false;
                    }else{
                        ESP_LOGE(GATTS_TABLE_TAG, "unknown descr value");
                        esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
                    }
                } else {
                  ESP_LOGE(GATTS_TABLE_TAG, "Handle this write event!!!");
                }
                /* send response when param->write.need_rsp is true*/
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }else{
                /* handle prepare write */
                example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT: 
            // the length of gattc prepare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX. 
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            example_exec_write_event_env(&prepare_write_env, param);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_log_buffer_hex(GATTS_TABLE_TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);

    	      is_connected = true;
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            is_connected = false;
            enable_location_act_ntf = false;
            enable_location_string_act_ntf = false;
            enable_orientation_sensor_ntf = false;
            enable_debug_ranges_act_ntf = false;
            enable_debug_particles_act_ntf = false;
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != IDX_NB){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to IDX_NB(%d)", param->add_attr_tab.num_handle, IDX_NB);
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(handle_table, param->add_attr_tab.handles, sizeof(handle_table));
                esp_ble_gatts_start_service(handle_table[IDX_SVC_POS]);
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(GATTS_TABLE_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == profile_tab[idx].gatts_if) {
                if (profile_tab[idx].gatts_cb) {
                    profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

esp_err_t ble_set_new_location() {

  esp_err_t ret;
  ret = esp_ble_gatts_set_attr_value(handle_table[IDX_CHAR_VAL_POS], sizeof(current_position), (uint8_t*) &current_position);
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "current_position esp_ble_gatts_set_attr_value failed: %d", ret);
    return ESP_FAIL;
  }

  if (enable_location_act_ntf) {
    ESP_LOGD(GATTS_TABLE_TAG, "enable_location_act_ntf");
    ret = esp_ble_gatts_send_indicate(profile_tab[PROFILE_APP_IDX].gatts_if, profile_tab[PROFILE_APP_IDX].conn_id, handle_table[IDX_CHAR_VAL_POS], sizeof(current_position), (uint8_t*) &current_position, false);
    if (ret) {
      ESP_LOGE(GATTS_TABLE_TAG, "set_new_orientation_act_value esp_ble_gatts_send_indicate failed: %d", ret);
      return ESP_FAIL;
    }
  }

    int len = sprintf(current_position_string, "(%d, %d, %d) (%d, %d, %d) - %d", 
        current_position.pos.x, current_position.pos.y, current_position.pos.z,
        current_position.std.x, current_position.std.y, current_position.std.z,
        current_position.is_valid);

    ret = esp_ble_gatts_set_attr_value(handle_table[IDX_CHAR_VAL_POS_STR], len, (uint8_t*) current_position_string);
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "current_position_string esp_ble_gatts_set_attr_value failed: %d", ret);
    return ESP_FAIL;
  }

  if (enable_location_string_act_ntf) {
    ESP_LOGD(GATTS_TABLE_TAG, "enable_location_string_act_ntf");
    ret = esp_ble_gatts_send_indicate(profile_tab[PROFILE_APP_IDX].gatts_if, profile_tab[PROFILE_APP_IDX].conn_id, handle_table[IDX_CHAR_VAL_POS_STR], sizeof(current_position_string), (uint8_t*)current_position_string, false);
    if (ret) {
      ESP_LOGE(GATTS_TABLE_TAG, "current_position_string esp_ble_gatts_send_indicate failed: %d", ret);
      return ESP_FAIL;
    }
  }
  return ESP_OK;
}

esp_err_t set_new_ranges(int ranges[6]) {

  esp_err_t ret;
  ret = esp_ble_gatts_set_attr_value(handle_table[IDX_CHAR_VAL_DEBUG_RANGES], sizeof(int) * 6, (uint8_t*) ranges);
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "current_position esp_ble_gatts_set_attr_value failed: %d", ret);
    return ESP_FAIL;
  }

  if (enable_debug_ranges_act_ntf) {
    ESP_LOGD(GATTS_TABLE_TAG, "enable_location_act_ntf");
    ret = esp_ble_gatts_send_indicate(profile_tab[PROFILE_APP_IDX].gatts_if, profile_tab[PROFILE_APP_IDX].conn_id, handle_table[IDX_CHAR_VAL_DEBUG_RANGES], sizeof(int) * 6, (uint8_t*) ranges, false);
    if (ret) {
      ESP_LOGE(GATTS_TABLE_TAG, "set_new_orientation_act_value esp_ble_gatts_send_indicate failed: %d", ret);
      return ESP_FAIL;
    }
  }

  return ESP_OK;
}

esp_err_t ble_push_particles(particle_t* particles, int length) {

//   esp_err_t ret;
  
//   if (length > 2) length = 2;
//   for (int i =0; i< length; i++)
//   {
//       memcpy((uint8_t*) &ble_particles[i*10], (uint8_t*) &particles[i], 10);
//   }

//   ret = esp_ble_gatts_set_attr_value(handle_table[IDX_CHAR_VAL_DEBUG_PARTICLES], length * 10, (uint8_t*)particles);

//   if (ret) {
//     ESP_LOGE(GATTS_TABLE_TAG, "current_position esp_ble_gatts_set_attr_value failed: %d", ret);
//     return ESP_FAIL;
//   }

//   if (enable_debug_particles_act_ntf) {
//     ESP_LOGD(GATTS_TABLE_TAG, "enable_location_act_ntf");
//     ret = esp_ble_gatts_send_indicate(profile_tab[PROFILE_APP_IDX].gatts_if, profile_tab[PROFILE_APP_IDX].conn_id, handle_table[IDX_CHAR_VAL_DEBUG_PARTICLES], length * 10, (uint8_t*)particles, false);
//     if (ret) {
//       ESP_LOGE(GATTS_TABLE_TAG, "set_new_orientation_act_value esp_ble_gatts_send_indicate failed: %d", ret);
//       return ESP_FAIL;
//     }
//   }

  return ESP_OK;

}

esp_err_t start_bluetooth(void)
{
    esp_err_t ret;



    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gatts register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gap register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gatts app register error, error code = %x", ret);
        return ret;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTS_TABLE_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
        return local_mtu_ret;
    }

    return ESP_OK;
}

void set_new_orientation_cb(new_orientation_cb_t cb) {
    new_orientation_cb = cb;
}
