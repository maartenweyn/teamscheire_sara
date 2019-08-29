#include "app_config.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"


#define TAG "APP_CONFIG:"

app_config_t app_config;

void create_default_config() {
  strcpy(app_config.wifi_ssid, "teamscheire");
  strcpy(app_config.wifi_passwd, "123456789");

  for (int index = 0; index < (sizeof(app_config.node_positions) / (sizeof(position_t))); ++index)
  {
    app_config.node_positions[index].x = -1;
    app_config.node_positions[index].y = -1;
  }  
}


esp_err_t save_config() {
  cJSON *config = cJSON_CreateObject();
  if (config == NULL) return ESP_FAIL;

  cJSON *wifi_ssid = cJSON_CreateString(app_config.wifi_ssid);
  if (wifi_ssid == NULL) goto end;
  cJSON_AddItemToObject(config, "wifi_ssid", wifi_ssid);

  cJSON *wifi_passwd = cJSON_CreateString(app_config.wifi_passwd);
  if (wifi_passwd == NULL) goto end;
  cJSON_AddItemToObject(config, "wifi_passwd", wifi_passwd);


  cJSON *node_positions = cJSON_CreateArray();
  if (node_positions == NULL) goto end;
  cJSON_AddItemToObject(config, "node_positions", node_positions);

  cJSON *x = NULL;
  cJSON *y = NULL;
  cJSON *position = NULL;

  for (int index = 0; index < (sizeof(app_config.node_positions) / (sizeof(position_t))); ++index)
  {
      position = cJSON_CreateObject();
      if (position == NULL) goto end;
      cJSON_AddItemToArray(node_positions, position);

      x = cJSON_CreateNumber(app_config.node_positions[index].x);
      if (x == NULL) goto end;
      cJSON_AddItemToObject(position, "x", x);

      y = cJSON_CreateNumber(app_config.node_positions[index].y);
      if (y == NULL) goto end;

      cJSON_AddItemToObject(position, "y", y);
  }

  char *string = cJSON_Print(config);

  ESP_LOGI(TAG, "Opening file %s", CONFIG_FILE);
  FILE* f = fopen(CONFIG_FILE, "w");
  if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file for writing");
  } else {
    fprintf(f, "%s\n", string);
    fclose(f);
  }
  ESP_LOGI(TAG, "File written with %s", string);


  cJSON_Delete(config);
  return ESP_OK;

end:
  cJSON_Delete(config);
  return ESP_FAIL;
}


esp_err_t load_config() {
  ESP_LOGI(TAG, "Opening file %s", CONFIG_FILE);
	char* read_buf=malloc(1024);

  FILE* f = fopen(CONFIG_FILE, "r");
  if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file for writing");
      create_default_config();
      return save_config();
  } else {
    while(1){
      uint32_t r;
    	r=fread(read_buf,1,1024,f);
    	if(r>0){
    		 ESP_LOGE(TAG, "Content: %s", read_buf);
    	}else
    		break;
    }

    cJSON *json = cJSON_Parse(read_buf);

    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        goto end;
    }

    const cJSON *wifi_ssid = cJSON_GetObjectItemCaseSensitive(json, "wifi_ssid");
    if (cJSON_IsString(wifi_ssid) && (wifi_ssid->valuestring != NULL))
    {
        ESP_LOGE(TAG, "wifi_ssid \"%s\"\n", wifi_ssid->valuestring);
        strcpy(app_config.wifi_ssid, wifi_ssid->valuestring);
    }

    const cJSON *wifi_passwd = cJSON_GetObjectItemCaseSensitive(json, "wifi_passwd");
    if (cJSON_IsString(wifi_passwd) && (wifi_passwd->valuestring != NULL))
    {
        ESP_LOGE(TAG, "wifi_passwd \"%s\"\n", wifi_passwd->valuestring);
        strcpy(app_config.wifi_passwd, wifi_passwd->valuestring);
    }

    const cJSON *nearby_threshold = cJSON_GetObjectItemCaseSensitive(json, "nearby_threshold");
    if (cJSON_IsNumber(nearby_threshold))
    {
      ESP_LOGE(TAG, "nearby_threshold \"%d\"\n", nearby_threshold->valueint);
      app_config.nearby_threshold =  nearby_threshold->valueint;
    } else {
      app_config.nearby_threshold =  DEF_NEARBY_THRESHOLD;
    }

    const cJSON *node_positions = cJSON_GetObjectItemCaseSensitive(json, "node_positions");
    const cJSON *position = NULL;
    int counter = 0;
    cJSON_ArrayForEach(position, node_positions)
    {
        cJSON *x = cJSON_GetObjectItemCaseSensitive(position, "x");
        cJSON *y = cJSON_GetObjectItemCaseSensitive(position, "y");

        if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y))
        {
            goto end;
        }

        app_config.node_positions[counter].x = x->valuedouble;
        app_config.node_positions[counter].y = y->valuedouble;

        ESP_LOGE(TAG, "node_position %d: %d, %d", counter,  app_config.node_positions[counter].x,  app_config.node_positions[counter].y);
        counter++;
    }

    fclose(f);

    return ESP_OK;
    cJSON_Delete(json);

end:
    cJSON_Delete(json);
    return ESP_FAIL;
  }
  
  return ESP_FAIL;
}
