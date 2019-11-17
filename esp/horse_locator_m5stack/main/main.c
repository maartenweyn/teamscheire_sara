
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include <sys/socket.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/dac.h"
//#include "eth.h"
///#include "event.h"
// #include "wifi.h"
//#include "hal_i2c.h"
//#include "hal_i2s.h"
//#include "wm8978.h"
#include "webserver.h"
// #include "http.h"
#include "cJSON.h"
// #include "mdns_task.h"
// #include "audio.h"
#include <dirent.h>
#include "esp_heap_caps.h"
// #include "euler.h"
// #include "websocket.h"
#include "esp_heap_caps.h"
// #include "aplay.h"
#include "esp_task_wdt.h"

#include "board.h"

#include "periph_sdcard.h"
#include "periph_led.h"
#include "periph_button.h"
#include "periph_service.h"

#include "input_key_service.h"

#include "app_sound.h"
#include "app_config.h"
#include "app_sdcard.h"
#include "localization.h"
#include "uwb_parser.h"
#include "app_leds.h"
#include "app_power.h"

#include "tftspi.h"
#include "tft.h"



 #include <http_server.h>


//#include "web_radio.h"


#define TAG "main:"
// typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
// typedef int (*http_cb) (http_parser*);


//char* http_body;

//#define GPIO_OUTPUT_IO_0    5
//#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_IO_0))

esp_periph_set_handle_t set;
esp_periph_handle_t led_handle = NULL;

volatile bool play_sound = false;
volatile bool store_config = false;

static char lcd_text[128];

static esp_err_t test_input_key_service_callback(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{

    ESP_LOGI(TAG, "type=>%d, source=>%d, data=>%d, len=>%d", evt->type, (int)evt->source, (int)evt->data, evt->len);

    if (evt->len == BUTTON_A) {
      if (evt->type == 2) {
        int batt = getBatteryLevel();
        ESP_LOGI(TAG, "Battery Level %d", batt);
        sprintf(lcd_text, "Batt. %d%%", batt);
        _fg = TFT_BLACK;
        _bg = TFT_WHITE;
        TFT_fillRoundRect(200, 50, 100, 20, 0, TFT_WHITE);
        TFT_print(lcd_text, 200, 50);
      } else if (evt->type == 4) { // long press 
        power_shutdown();
      }
    }

    if (evt->len == BUTTON_C) {
      if (evt->type == 2) { // short press 
        if (app_config.volume > 4) {
          app_config.volume -= 5;
          set_volume();
          ESP_LOGI(TAG, "Volume down %d", app_config.volume);
          sprintf(lcd_text, "Vol: %d%%", app_config.volume);
          _fg = TFT_BLACK;
          _bg = TFT_WHITE;
          TFT_fillRoundRect(10, 50, 100, 20, 0, _bg);
          TFT_print(lcd_text, 10, 50);
          play_sound = true;
        }
      } else if (evt->type == 4) { // long press 
        store_config = true;
      }
    }

    if (evt->len == BUTTON_B) {
      if (evt->type == 2) { // short press 
        if (app_config.volume < 96) {
          app_config.volume += 5;
          set_volume();
          ESP_LOGI(TAG, "Volume up %d", app_config.volume);
          sprintf(lcd_text, "Vol: %d%%", app_config.volume);
          _fg = TFT_BLACK;
          _bg = TFT_WHITE;
          TFT_fillRoundRect(10, 50, 100, 20, 0, _bg);
          TFT_print(lcd_text, 10, 50);
          play_sound = true;
        }
      } else if (evt->type == 4) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        power_shutdown();
      }
    }
    return ESP_OK;
}

#define SPI_BUS TFT_HSPI_HOST

void lcdInit(void){
        //test_sd_card();
    // ========  PREPARE DISPLAY INITIALIZATION  =========
    esp_err_t ret;

    // ====================================================================
    // === Pins MUST be initialized before SPI interface initialization ===
    // ====================================================================
    TFT_PinsInit();

    // ====  CONFIGURE SPI DEVICES(s)  ====================================================================================

    spi_lobo_device_handle_t spi;
	
    spi_lobo_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,				// set SPI MISO pin
        .mosi_io_num=PIN_NUM_MOSI,				// set SPI MOSI pin
        .sclk_io_num=PIN_NUM_CLK,				// set SPI CLK pin
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
		.max_transfer_sz = 6*1024,
    };
    spi_lobo_device_interface_config_t devcfg={
        .clock_speed_hz=8000000,                // Initial clock out at 8 MHz
        .mode=0,                                // SPI mode 0
        .spics_io_num=-1,                       // we will use external CS pin
		.spics_ext_io_num=PIN_NUM_CS,           // external CS pin
		.flags=LB_SPI_DEVICE_HALFDUPLEX,        // ALWAYS SET  to HALF DUPLEX MODE!! for display spi
    };

    vTaskDelay(500 / portTICK_RATE_MS);
    printf("Pins used: miso=%d, mosi=%d, sck=%d, cs=%d\r\n", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);


	// ==================================================================
	// ==== Initialize the SPI bus and attach the LCD to the SPI bus ====

	ret=spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
    assert(ret==ESP_OK);
	printf("SPI: display device added to spi bus (%d)\r\n", SPI_BUS);
	disp_spi = spi;

	// ==== Test select/deselect ====
	ret = spi_lobo_device_select(spi, 1);
    assert(ret==ESP_OK);
	ret = spi_lobo_device_deselect(spi);
    assert(ret==ESP_OK);

	printf("SPI: attached display device, speed=%u\r\n", spi_lobo_get_speed(spi));
	printf("SPI: bus uses native pins: %s\r\n", spi_lobo_uses_native_pins(spi) ? "true" : "false");


	printf("SPI: display init...\r\n");
	TFT_display_init();
    printf("OK\r\n");
	// ---- Detect maximum read speed ----
	max_rdclock = 40000000;
	printf("SPI: Max rd speed = %u\r\n", max_rdclock);

    // ==== Set SPI clock used for display operations ====
	spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);
	printf("SPI: Changed speed to %u\r\n", spi_lobo_get_speed(spi));

  font_rotate = 0;
	text_wrap = 0;
	font_transparent = 0;
	font_forceFixed = 0;
	gray_scale = 0;
  _fg = TFT_BLACK;
  _bg = TFT_WHITE;
  TFT_invertDisplay(INVERT_ON);
  TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
	TFT_setRotation(LANDSCAPE_FLIP);
	TFT_setFont(DEJAVU24_FONT, NULL);
	TFT_resetclipwin();
  TFT_fillScreen(TFT_WHITE);

  TFT_fillRect(0, 110, DEFAULT_TFT_DISPLAY_WIDTH, 60, TFT_CYAN);


  _bg = TFT_CYAN;
  _fg = TFT_WHITE;
  TFT_print("SarAssist", CENTER, CENTER);
  TFT_print("Team Scheire", CENTER, LASTY+25);

  _fg = TFT_BLACK;
  _bg = TFT_WHITE;

  
	TFT_setFont(SMALL_FONT, NULL);

  TFT_print("VOL DOWN", 27, 2);
  TFT_print("VOL UP", 138, 2);
  TFT_print("STORE CONFIG", 10, 20);

  TFT_print("BATTERY", 230, 2);
  TFT_print("POWER OFF", 225, 20);

  
	TFT_setFont(DEJAVU18_FONT, NULL);

  sprintf(lcd_text, "Vol: %d%%", app_config.volume);
  TFT_fillRoundRect(10, 50, 100, 20, 0, TFT_WHITE);
  TFT_print(lcd_text, 10, 50);

  TFT_fillRect(0, 205, DEFAULT_TFT_DISPLAY_WIDTH, 25, TFT_LIGHTGREY);

  // TFT_fillRect(10, 210, 100, 20, TFT_WHITE);
  // TFT_print("No Ranges", 10, 210);

  //disp_deselect();
}


void app_main()
{
  //event_engine_init();

  //ESP_ERROR_CHECK(nvs_flash_erase());

  esp_err_t err = nvs_flash_init();
  ESP_LOGI(TAG, "nvs_flash_init %x", err);
  if (err == ESP_ERR_NVS_NO_FREE_PAGES  || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
      ESP_LOGI(TAG, "nvs_flash_init %x", err);
  }

  // esp_task_wdt_init(2, false);
  // esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));
  // esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1));

  esp_periph_config_t periph_cfg = DEFAULT_ESP_PHERIPH_SET_CONFIG();
  set = esp_periph_set_init(&periph_cfg);

  ESP_LOGI(TAG, "[ 1 ] Mount sdcard");
  ESP_LOGI(TAG, "[1.1] Start and wait for SDCARD to mount");

  #ifndef SKIP_SD_CARD
  init_sdcard();
  #endif

  load_config();

  init_letter_storage();
  save_sound('A');
  save_sound('F');
  save_sound('B');
  save_sound('M');
  save_sound('C');
  save_sound('H');
  save_sound('E');
  save_sound('K');
  save_sound('X');

  deinit_sdcard();


  lcdInit();

  ESP_LOGI(TAG, "[ 3 ] Init leds");

  leds_init();
  leds_setcolor(255, 0, 0, 0);

  leds_blink(0, 0, 255, 0, 500);


  ESP_LOGI(TAG, "[ 2 ] Setup Audio");

  setup_player();
  

  // Initialize Button peripheral
  periph_button_cfg_t btn_cfg = {
      .gpio_mask = (1ULL << BUTTON_A | (1ULL << BUTTON_B) | (1ULL << BUTTON_C)), 
  };
  esp_periph_handle_t button_handle = periph_button_init(&btn_cfg);
  esp_periph_start(set, button_handle);

  input_key_service_info_t input_info[] = {      
        {                                   
            .type = PERIPH_ID_BUTTON,      
            .user_id = 0,         
            .act_id = BUTTON_A,         
        },                                  
        {                                   
            .type = PERIPH_ID_BUTTON,      
            .user_id = 1,        
            .act_id = BUTTON_B,        
        },                                  
        {                                   
            .type = PERIPH_ID_BUTTON,      
            .user_id = 2,         
            .act_id = BUTTON_C,         
        }                                  
    };

  periph_service_handle_t input_key_handle = input_key_service_create(set);

  input_key_service_add_key(input_key_handle, input_info, INPUT_KEY_NUM);
  periph_service_set_callback(input_key_handle, test_input_key_service_callback, NULL);


  static httpd_handle_t server = NULL;
  initialise_wifi(&server);

  init_debug_log();

  initialize_localization_engine();

  uwb_parser_init();   

  //disable node speaker
  // esp_err_t ret = dac_output_voltage(DAC_CHANNEL_1, 0);
  // ESP_LOGI(TAG, "dac_output_voltage %d", ret);

  int batt = getBatteryLevel();
  ESP_LOGI(TAG, "Battery Level %d", batt);
  sprintf(lcd_text, "Batt. %d%%", batt);
  TFT_fillRoundRect(200, 50, 100, 20, 0, TFT_WHITE);
  TFT_print(lcd_text, 200, 50);

  //power_shutdown();


  xTaskCreate(locator_task, "locator_task", 4096, NULL, 5, NULL);
  // //xTaskCreatePinnedToCore(locator_task, "locator_task", 4096, NULL, 5, NULL, 1);
  // //xTaskCreate(webserver_task, "webserver_task", 4096, NULL, 10, NULL);
  // //xTaskCreatePinnedToCore(webserver_task, "webserver_task", 4096, NULL, 10, NULL, 0);
  vTaskSuspend(NULL);

  //never goto here
  while(1){
      ESP_LOGI(TAG, "should not be here");
  }
}

