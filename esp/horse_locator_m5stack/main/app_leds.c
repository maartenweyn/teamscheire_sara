#include "app_leds.h"

#include "esp_timer.h"

#include "esp32_digital_led_lib.h"

//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#define TAG "LEDS"


static strand_t STRANDS[] = { // Avoid using any of the strapping pins on the ESP32
  {.rmtChannel = 1, .gpioNum = 15, .ledType = LED_SK6812_V1, .brightLimit = 100, .numPixels =  12,
   .pixels = 0, ._stateVars = 0},
};

static int STRANDCNT = sizeof(STRANDS)/sizeof(STRANDS[0]);
static pixelColor_t colTarget[64];

static esp_timer_handle_t oneshot_timer;
static volatile bool blinking = false;

static void oneshot_timer_callback(void* arg);



static void oneshot_timer_callback(void* arg) {
  leds_setcolor(0, 0, 0, 0);
  /* To start the timer which is running, need to stop it first */
  esp_timer_stop(oneshot_timer);
  //esp_timer_delete(oneshot_timer);
  blinking = false;
  ESP_LOGD(TAG, "blinking false");
}

static void leds_init_timer() {
  const esp_timer_create_args_t oneshot_timer_args = {
          .callback = &oneshot_timer_callback,
          /* argument specified here will be passed to timer callback function */
          .arg = 0,
          .name = "one-shot"
  };
   
  ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));

}

void leds_init() {
  if (digitalLeds_initStrands(STRANDS, STRANDCNT)) {
		ESP_LOGE(TAG, "Init FAILURE: halting");
		while (true) {};
	}

  leds_init_timer();
}

void leds_setcolor(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  for (uint16_t i = 0; i < STRANDS[0].numPixels; i++) {
        colTarget[i] = pixelFromRGBW(r, g, b, w);
    }
    for (uint16_t i = 0; i < STRANDS[0].numPixels; i++) {
		STRANDS[0].pixels[i] = colTarget[i];
	}
	digitalLeds_updatePixels(&STRANDS[0]);
}

void leds_blink(uint8_t r, uint8_t g, uint8_t b, uint8_t w, int delay_ms) {
  if (blinking) {
    ESP_LOGD(TAG, "Timer still running");

    blinking = false; //hack othersize hangs on page load
    leds_init_timer();
    return;
  }

  blinking = true;

  ESP_LOGD(TAG, "blinking true");

  leds_setcolor(r, g, b, w);
  
 
  esp_timer_start_once(oneshot_timer, delay_ms * 1000);
  ESP_LOGD(TAG, "timer started, time since boot: %lld us", esp_timer_get_time());

}