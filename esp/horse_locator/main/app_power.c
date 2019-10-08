#include "app_power.h"

#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "board_pins_config.h"

#define TAG "POWER:"

int8_t getBatteryLevel()
{
  int res = 0;
	uint8_t data = 0xFF;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  res |= i2c_master_start(cmd);
  res |= i2c_master_write_byte(cmd, (IP5306_ADDR<<1) | I2C_MASTER_WRITE, 1);
  res |= i2c_master_write_byte(cmd, IP5306_REG_READ3, 1);
  res |= i2c_master_start(cmd);
  res |= i2c_master_write_byte(cmd, ( IP5306_ADDR << 1 ) | I2C_MASTER_READ, 1);
  res |= i2c_master_read_byte(cmd, &data, 0);
  res |= i2c_master_stop(cmd);
  res |= i2c_master_cmd_begin(BOARD_I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);

  if (res == 0) {
    ESP_LOGI(TAG, "getBatteryLevel %x (%d)", data, res);

    switch (data & 0xF0) {
      case 0xE0: return 25;
      case 0xC0: return 50;
      case 0x80: return 75;
      case 0x00: return 100;
      default: return 0;
    }
  } else {
    ESP_LOGE(TAG, "Battery level cannot be read");
    return -1;
  }
}