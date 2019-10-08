#include "app_leds.h"
#include "ws2812.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <stdio.h>

#define LED_STRIP_LENGTH  19
#define LED_STRIP_DIVIDE  4

#define TAG "LEDS:"

#define delay_ms(ms) vTaskDelay((ms) / portTICK_RATE_MS)

rgbVal pixels[LED_STRIP_LENGTH];

void leds_setcolor(uint8_t div, uint8_t r, uint8_t g, uint8_t b) {
  rgbVal color = makeRGBVal(r, g, b);
  for (uint8_t i = 0; i < LED_STRIP_LENGTH; i++) {
    if (i % div == 0)
      pixels[i] = color;
    else
      pixels[i] = makeRGBVal(0, 0, 0);
  }

  ws2812_setColors(pixels);
}

void leds_init() {
  ws2812_init(22, LED_STRIP_LENGTH);
  leds_setcolor(LED_STRIP_DIVIDE, 100, 0, 0);
}

// static struct led_color_t led_strip_buf_1[LED_STRIP_LENGTH];
// static struct led_color_t led_strip_buf_2[LED_STRIP_LENGTH];

// void leds_init() {
//   struct led_strip_t led_strip = {
//       .rgb_led_type = RGB_LED_TYPE_WS2812,
//       .rmt_channel = RMT_CHANNEL_1,
//       .rmt_interrupt_num = 18,
//       .gpio = GPIO_NUM_22,
//       .led_strip_buf_1 = led_strip_buf_1,
//       .led_strip_buf_2 = led_strip_buf_2,
//       .led_strip_length = LED_STRIP_LENGTH
//   };
//   led_strip.access_semaphore = xSemaphoreCreateBinary();

//   bool led_init_ok = led_strip_init(&led_strip);
  
//   ESP_LOGI(TAG, "led_strip_init %d", led_init_ok);

//   led_strip_set_pixel_rgb(&led_strip, 1, 255, 0, 0);
//   led_strip_set_pixel_rgb(&led_strip, 2, 0, 255, 0);
//   led_strip_set_pixel_rgb(&led_strip, 3, 0, 0, 255);

//   led_strip_show(&led_strip);
// }