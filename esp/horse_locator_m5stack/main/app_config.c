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
    app_config.node_positions[index].y = -1;
  }  
}


esp_err_t save_config() {
  cJSON *x = NULL;
  cJSON *y = NULL;
  cJSON *z = NULL;
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

  // SENSOR HEIGHT
  cJSON *sensor_height  = cJSON_CreateNumber(app_config.sensor_height);
  if (sensor_height == NULL) goto end;
  cJSON_AddItemToObject(config, "sensor_height", sensor_height);

    // SENSOR HEIGHT
  cJSON *volume  = cJSON_CreateNumber(app_config.volume);
  if (volume == NULL) goto end;
  cJSON_AddItemToObject(config, "volume", volume);

  // DEBUG
  cJSON *store_ranges  = cJSON_CreateNumber(app_config.store_ranges);
  if (store_ranges == NULL) goto end;
  cJSON_AddItemToObject(config, "store_ranges", store_ranges);

    // DEBUG
  cJSON *store_range_counter  = cJSON_CreateNumber(app_config.store_range_counter);
  if (store_range_counter == NULL) goto end;
  cJSON_AddItemToObject(config, "store_range_counter", store_range_counter);

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

      z = cJSON_CreateNumber(app_config.node_positions[index].z);
      if (z == NULL) goto end;

      cJSON_AddItemToObject(position, "z", z);
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

  // PARTICLE FILTEr
  cJSON *nr_of_particles = cJSON_CreateNumber(app_config.particle_filter.nr_of_particles);
  if (nr_of_particles == NULL) goto end;
  cJSON_AddItemToObject(config, "nr_of_particles", nr_of_particles);

  cJSON *max_speed = cJSON_CreateNumber(app_config.particle_filter.max_speed);
  if (max_speed == NULL) goto end;
  cJSON_AddItemToObject(config, "max_speed", max_speed);
  
  cJSON *UWB_std2 = cJSON_CreateNumber(app_config.particle_filter.UWB_std2);
  if (UWB_std2 == NULL) goto end;
  cJSON_AddItemToObject(config, "UWB_std2", UWB_std2);

  cJSON *std_min_threshold = cJSON_CreateNumber(app_config.particle_filter.std_min_threshold);
  if (std_min_threshold == NULL) goto end;
  cJSON_AddItemToObject(config, "std_min_threshold", std_min_threshold);



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
    	if (r>0) {
    		 ESP_LOGI(TAG, "Content: %s", read_buf);
    	} else
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
        //goto end;
    }

    const cJSON *wifi_ssid = cJSON_GetObjectItemCaseSensitive(json, "wifi_ssid");
    if (cJSON_IsString(wifi_ssid) && (wifi_ssid->valuestring != NULL))
    {
        ESP_LOGI(TAG, "wifi_ssid %s", wifi_ssid->valuestring);
        strcpy(app_config.wifi_ssid, wifi_ssid->valuestring);
    } else {
      strcpy(app_config.wifi_ssid, DEF_SSID);
    }

    const cJSON *wifi_passwd = cJSON_GetObjectItemCaseSensitive(json, "wifi_passwd");
    if (cJSON_IsString(wifi_passwd) && (wifi_passwd->valuestring != NULL))
    {
        ESP_LOGI(TAG, "wifi_passwd %s", wifi_passwd->valuestring);
        strcpy(app_config.wifi_passwd, wifi_passwd->valuestring);
    }else {
      strcpy(app_config.wifi_passwd, DEF_passwd);
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


    const cJSON *sensor_height = cJSON_GetObjectItemCaseSensitive(json, "sensor_height");
    if (cJSON_IsNumber(sensor_height))
    {
      ESP_LOGI(TAG, "sensor_height %d", sensor_height->valueint);
      app_config.sensor_height =  sensor_height->valueint;
    } else {
      app_config.sensor_height =  DEF_SENSOR_HEIGHT;
    }

    const cJSON *volume = cJSON_GetObjectItemCaseSensitive(json, "volume");
    if (cJSON_IsNumber(volume))
    {
      ESP_LOGI(TAG, "volume %d", volume->valueint);
      app_config.volume =  volume->valueint;
    } else {
      app_config.volume =  DEF_VOLUME;
    }

    const cJSON *node_positions = cJSON_GetObjectItemCaseSensitive(json, "node_positions");
    const cJSON *position = NULL;
    int counter = 0;
    cJSON_ArrayForEach(position, node_positions)
    {
        cJSON *x = cJSON_GetObjectItemCaseSensitive(position, "x");
        cJSON *y = cJSON_GetObjectItemCaseSensitive(position, "y");
        cJSON *z = cJSON_GetObjectItemCaseSensitive(position, "z");

        if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y))
        {
            goto end;
        }

        app_config.node_positions[counter].x = x->valuedouble;
        app_config.node_positions[counter].y = y->valuedouble;

        if (cJSON_IsNumber(z))
        {
          app_config.node_positions[counter].z = z->valuedouble;
        } else {
          app_config.node_positions[counter].z = DEF_NODE_HEIGHT;
        }


        ESP_LOGI(TAG, "node_position %d: %d, %d, %d", counter,  app_config.node_positions[counter].x,  app_config.node_positions[counter].y,  app_config.node_positions[counter].z);
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

    const cJSON *nr_of_particles = cJSON_GetObjectItemCaseSensitive(json, "nr_of_particles");
    if (cJSON_IsNumber(nr_of_particles))
    {
      ESP_LOGI(TAG, "nr_of_particles %d", nr_of_particles->valueint);
      app_config.particle_filter.nr_of_particles =  nr_of_particles->valueint;
    } else {
      app_config.particle_filter.nr_of_particles =  DEF_NR_OF_PARTICLES;
    }

    const cJSON *max_speed = cJSON_GetObjectItemCaseSensitive(json, "max_speed");
    if (cJSON_IsNumber(max_speed))
    {
      ESP_LOGI(TAG, "max_speed %d", nr_of_particles->valueint);
      app_config.particle_filter.max_speed =  nr_of_particles->valueint;
    } else {
      app_config.particle_filter.max_speed =  DEF_MAX_SPEED;
    }

    const cJSON *UWB_std2 = cJSON_GetObjectItemCaseSensitive(json, "UWB_std2");
    if (cJSON_IsNumber(UWB_std2))
    {
      ESP_LOGI(TAG, "UWB_std2 %d", UWB_std2->valueint);
      app_config.particle_filter.UWB_std2 =  UWB_std2->valueint;
    } else {
      app_config.particle_filter.UWB_std2 =  DEF_UWB_STD2;
    }

    const cJSON *std_min_threshold = cJSON_GetObjectItemCaseSensitive(json, "std_min_threshold");
    if (cJSON_IsNumber(std_min_threshold))
    {
      ESP_LOGI(TAG, "std_min_threshold %d", std_min_threshold->valueint);
      app_config.particle_filter.std_min_threshold =  std_min_threshold->valueint;
    } else {
      app_config.particle_filter.std_min_threshold =  DEF_STD_THRESH;
    }

    const cJSON *store_ranges = cJSON_GetObjectItemCaseSensitive(json, "store_ranges");
    if (cJSON_IsNumber(store_ranges))
    {
      ESP_LOGI(TAG, "store_ranges %d", store_ranges->valueint);
      app_config.store_ranges =  store_ranges->valueint;
    } else {
      app_config.store_ranges =  0;
    }

    const cJSON *store_range_counter = cJSON_GetObjectItemCaseSensitive(json, "store_range_counter");
    if (cJSON_IsNumber(store_range_counter))
    {
      ESP_LOGI(TAG, "store_range_counter %d", store_range_counter->valueint);
      app_config.store_range_counter =  store_range_counter->valueint;
    } else {
      app_config.store_range_counter =  0;
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
