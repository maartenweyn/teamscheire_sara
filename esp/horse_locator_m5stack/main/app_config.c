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
  cJSON *x = NULL;
  cJSON *y = NULL;
  cJSON *position = NULL;
  cJSON *letter = NULL;
  cJSON *letter_char = NULL;

  cJSON *config = cJSON_CreateObject();
  if (config == NULL) return ESP_FAIL;

  cJSON *wifi_ssid = cJSON_CreateString(app_config.wifi_ssid);
  if (wifi_ssid == NULL) goto end;
  cJSON_AddItemToObject(config, "wifi_ssid", wifi_ssid);

  cJSON *wifi_passwd = cJSON_CreateString(app_config.wifi_passwd);
  if (wifi_passwd == NULL) goto end;
  cJSON_AddItemToObject(config, "wifi_passwd", wifi_passwd);

  cJSON *nearby_threshold = cJSON_CreateNumber(app_config.nearby_threshold);
  if (nearby_threshold == NULL) goto end;
  cJSON_AddItemToObject(config, "nearby_threshold", nearby_threshold);

  // FIELD_SIZE
  cJSON *field_size = cJSON_CreateObject();
  if (field_size == NULL) goto end;
  cJSON_AddItemToObject(config, "field_size", field_size);

  x = cJSON_CreateNumber(app_config.field_size.x);
  if (x == NULL) goto end;
  cJSON_AddItemToObject(field_size, "x", x);

  y = cJSON_CreateNumber(app_config.field_size.y);
  if (y == NULL) goto end;
  cJSON_AddItemToObject(field_size, "y", y);

  // FIELD_SIZE_MARGIN
  cJSON *field_size_margin = cJSON_CreateNumber(app_config.field_size_margin);
  if (field_size_margin == NULL) goto end;
  cJSON_AddItemToObject(config, "field_size_margin", field_size_margin);

  //NODE_POSITIONS
  cJSON *node_positions = cJSON_CreateArray();
  if (node_positions == NULL) goto end;
  cJSON_AddItemToObject(config, "node_positions", node_positions);

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

  //LETTERS
  cJSON *letters = cJSON_CreateArray();
  if (letters == NULL) goto end;
  cJSON_AddItemToObject(config, "letters", letters);

  for (int index = 0; index < NR_OF_LETTERS; ++index)
  {
      letter = cJSON_CreateObject();
      if (letter == NULL) goto end;
      cJSON_AddItemToArray(letters, letter);

      x = cJSON_CreateNumber(app_config.letters[index].x);
      if (x == NULL) goto end;
      cJSON_AddItemToObject(letter, "x", x);

      y = cJSON_CreateNumber(app_config.letters[index].y);
      if (y == NULL) goto end;
      cJSON_AddItemToObject(letter, "y", y);

      char letter_string [5];
      sprintf(letter_string, "%c", app_config.letters[index].letter);
      letter_char = cJSON_CreateString(letter_string);
      if (letter_char == NULL) goto end;
      cJSON_AddItemToObject(letter, "letter", letter_char);
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
	char* read_buf=malloc(2048);
  memset(read_buf, 0, 2048);

  FILE* f = fopen(CONFIG_FILE, "r");
  if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file for writing");
      create_default_config();
      return save_config();
  } else {
    while(1){
      uint32_t r;
    	r=fread(read_buf,1,2048,f);
    	if(r>0){
    		 ESP_LOGI(TAG, "Content: %s", read_buf);
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
        ESP_LOGI(TAG, "wifi_ssid %s", wifi_ssid->valuestring);
        strcpy(app_config.wifi_ssid, wifi_ssid->valuestring);
    }

    const cJSON *wifi_passwd = cJSON_GetObjectItemCaseSensitive(json, "wifi_passwd");
    if (cJSON_IsString(wifi_passwd) && (wifi_passwd->valuestring != NULL))
    {
        ESP_LOGI(TAG, "wifi_passwd %s", wifi_passwd->valuestring);
        strcpy(app_config.wifi_passwd, wifi_passwd->valuestring);
    }

    const cJSON *nearby_threshold = cJSON_GetObjectItemCaseSensitive(json, "nearby_threshold");
    if (cJSON_IsNumber(nearby_threshold))
    {
      ESP_LOGI(TAG, "nearby_threshold %d", nearby_threshold->valueint);
      app_config.nearby_threshold =  nearby_threshold->valueint;
    } else {
      app_config.nearby_threshold =  DEF_NEARBY_THRESHOLD;
    }

    const cJSON *field_size = cJSON_GetObjectItemCaseSensitive(json, "field_size");
    cJSON *field_size_x = cJSON_GetObjectItemCaseSensitive(field_size, "x");
    if (cJSON_IsNumber(field_size_x))
    {
      ESP_LOGI(TAG, "field_size_x %d", field_size_x->valueint);
      app_config.field_size.x =  field_size_x->valueint;
    } else {
      app_config.field_size.x =  DEF_FIELD_SIZE_X;
    }
    cJSON *field_size_y = cJSON_GetObjectItemCaseSensitive(field_size, "y");
    if (cJSON_IsNumber(field_size_y))
    {
      ESP_LOGI(TAG, "field_size_y %d", field_size_y->valueint);
      app_config.field_size.y =  field_size_y->valueint;
    } else {
      app_config.field_size.y =  DEF_FIELD_SIZE_Y;
    }

    const cJSON *field_size_margin = cJSON_GetObjectItemCaseSensitive(json, "field_size_margin");
    if (cJSON_IsNumber(field_size_margin))
    {
      ESP_LOGI(TAG, "field_size_margin %d", field_size_margin->valueint);
      app_config.field_size_margin =  field_size_margin->valueint;
    } else {
      app_config.field_size_margin =  DEF_FIELD_SIZE_MARGIN;
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

        ESP_LOGI(TAG, "node_position %d: %d, %d", counter,  app_config.node_positions[counter].x,  app_config.node_positions[counter].y);
        counter++;
    }

    const cJSON *letters = cJSON_GetObjectItemCaseSensitive(json, "letters");
    const cJSON *letter = NULL;
    counter = 0;
    cJSON_ArrayForEach(letter, letters)
    {
        cJSON *x = cJSON_GetObjectItemCaseSensitive(letter, "x");
        cJSON *y = cJSON_GetObjectItemCaseSensitive(letter, "y");
        cJSON *letter_char = cJSON_GetObjectItemCaseSensitive(letter, "letter");

        if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y))
        {
            goto end;
        }

        app_config.letters[counter].x = x->valuedouble;
        app_config.letters[counter].y = y->valuedouble;
        app_config.letters[counter].letter = letter_char->valuestring[0];

        ESP_LOGI(TAG, "letter %c: %d, %d", app_config.letters[counter].letter,  app_config.letters[counter].x,  app_config.letters[counter].y);
        counter++;
    }

    fclose(f);
    free(read_buf);
    cJSON_Delete(json);

    return ESP_OK;

end:
    cJSON_Delete(json);
    free(read_buf);
    return ESP_FAIL;
  }
  
  free(read_buf);
  return ESP_FAIL;
}