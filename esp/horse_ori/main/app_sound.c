#include "app_sound.h"


#include <stdio.h>
#include "esp_log.h"
#include "aplay.h"


#define TAG "APP_SOUND:"

void play_letter(char l) {
  char file_name[50];
  sprintf(file_name, "/sdcard/audio/%c.raw", l);
  ESP_LOGI(TAG, "file %s", file_name);
  aplay_raw(file_name);
}