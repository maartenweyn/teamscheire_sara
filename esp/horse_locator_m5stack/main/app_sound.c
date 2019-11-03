#include "app_sound.h"


#include <stdio.h>
#include "esp_log.h"
//#include "aplay.h"

#include "esp_audio.h"
#include "audio_element.h"
#include "audio_event_iface.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "esp_peripherals.h"
#include "periph_sdcard.h"
#include "raw_stream.h"
#include "wav_encoder.h"
#include "wav_decoder.h"
#include "fatfs_stream.h"
#include "audio_mem.h"
#include "main.h"


#ifdef  __WM8978_H 
#include "wm8978.h"
#else
#include "es8388.h"
#endif

#define TAG "APP_SOUND:"

static esp_audio_handle_t player;

static void esp_audio_state_task (void *para)
{
    QueueHandle_t que = (QueueHandle_t) para;
    esp_audio_state_t esp_state = {0};
    while (1) {
        xQueueReceive(que, &esp_state, portMAX_DELAY);
        ESP_LOGI(TAG, "esp_audio status:%x,err:%x\n", esp_state.status, esp_state.err_msg);
    }
    vTaskDelete(NULL);
}




static int audio_mute(bool mute) {
    int res = 0;

 #ifdef  __WM8978_H    
    if (mute) {
        res = wm8978_write_reg(1, 0x40);
        ESP_LOGD(TAG, "wm8978 REG 1: %d 0X%X", res, 0x40);

        res |= wm8978_write_reg(2, 0x0);
        ESP_LOGD(TAG, "wm8978 REG 2: %d 0X%X", res, 0x0);

        res |= wm8978_write_reg(3, 0x0);
        ESP_LOGD(TAG, "wm8978 REG 3: %d 0X%X", res, 0x0);
    } else {
        res = wm8978_write_reg(1, 0x2B);
        ESP_LOGD(TAG, "wm8978 REG 1: %d 0X%X", res, 0x2B);

        res |= wm8978_write_reg(2, 0x180);
        ESP_LOGD(TAG, "wm8978 REG 2: %d 0X%X", res, 0x180);

        res |= wm8978_write_reg(3, 0x6F);
        ESP_LOGD(TAG, "wm8978 REG 3: %d 0X%X", res, 0x180);
    }
#else

    res = es8388_set_voice_mute(mute);
#endif

    return res;
}


audio_err_t esp_player_music_play(const char *url)
{
    //AUDIO_MEM_CHECK(TAG, player, return ESP_ERR_AUDIO_MEMORY_LACK);
    esp_audio_state_t st = {0};
    int ret = ESP_OK;
    esp_audio_state_get(player, &st);
    if (st.status == AUDIO_STATUS_RUNNING) {
        ret = esp_audio_stop(player, TERMINATION_TYPE_NOW);
        if (ret != ESP_ERR_AUDIO_NO_ERROR) {
            return ret;
        }
    }
    //ret = esp_audio_media_type_set(player, type);
    ret |=  esp_audio_play(player, AUDIO_CODEC_TYPE_DECODER, url, 0);
    //ret = esp_audio_sync_play(player, url, 0);
    return ret;
}

void aplay_raw(char* filename){
    //esp_audio_vol_set(player, 40);
    ESP_LOGI(TAG, "aplay_raw wake");
    audio_mute(false);

    ESP_LOGI(TAG, "aplay_raw open file");
	FILE *f= fopen(filename, "r");
	if (f == NULL) {
			ESP_LOGE(TAG,"Failed to open file:%s",filename);
			return;
	}
	int rlen;
	//int res;
	char* samples_data = malloc(1024);
    size_t bytes_written = 0;
    ESP_LOGI(TAG, "aplay_raw read file");
	do {
		rlen=fread(samples_data,1,1024,f);
        //ESP_LOGI(TAG, "aplay_raw read %d bytes", rlen);
		//datalen-=rlen;
		//hal_i2s_write(0,samples_data,rlen,5000);
        //res = 
        i2s_write(0, samples_data, rlen, &bytes_written, 1000);

        //ESP_LOGI(TAG, "aplay_raw i2s write %d, %d bytes", res, bytes_written);
	} while(rlen==1024);
	fclose(f);
	free(samples_data);
	f=NULL;
    //esp_audio_vol_set(player, 0);

    ESP_LOGI(TAG, "aplay_raw sleep");
    audio_mute(true);
}

void play_letter(char l) {
  char file_name[50];
  sprintf(file_name, "/sdcard/audio/%c.raw", l);
  ESP_LOGI(TAG, "file %s", file_name);
  //audio_err_t ret = esp_player_music_play(file_name);
  aplay_raw(file_name);
  //ESP_LOGI(TAG, "esp_audio_play %d", ret);
}


// void setup_player2()
// {
//     //wm8978_init(NULL);
//     audio_board_handle_t board_handle = audio_board_init();

//     i2s_config_t i2s_config = {
//         .mode = I2S_MODE_MASTER  | I2S_MODE_TX | I2S_MODE_RX,
//         .sample_rate =  44100,
//         .bits_per_sample = 16,
//         .communication_format = I2S_COMM_FORMAT_I2S,
//         .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
//         .intr_alloc_flags = 0,
//         .dma_buf_count = 3,
//         .dma_buf_len = 1024,                                    
//         .use_apll = 1,                                                          
//         .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2
//     };
//     //install and start i2s driver
//     i2s_driver_install(0, &i2s_config, 0, NULL);

//     i2s_pin_config_t i2s_pin_cfg = {0};
//     get_i2s_pins(0, &i2s_pin_cfg);
//     i2s_set_pin(0, &i2s_pin_cfg);
// }

void setup_player(void)
{
    if (player ) {
        return ;
    }
    esp_audio_cfg_t cfg = {
        .in_stream_buf_size = 10 * 1024,
        .out_stream_buf_size = 6 * 1024,
        .evt_que = NULL,
        .resample_rate = 41000,
        .hal = NULL,
    };


    audio_board_handle_t board_handle = audio_board_init();
    cfg.hal = board_handle->audio_hal;
    cfg.evt_que = xQueueCreate(3, sizeof(esp_audio_state_t));
    audio_hal_ctrl_codec(cfg.hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    player = esp_audio_create(&cfg);
    xTaskCreate(esp_audio_state_task, "player_task", 4096, cfg.evt_que, 1, NULL);

    // Create readers and add to esp_audio
    // fatfs_stream_cfg_t fs_reader = FATFS_STREAM_CFG_DEFAULT();
    // fs_reader.type = AUDIO_STREAM_READER;
    i2s_stream_cfg_t i2s_reader = I2S_STREAM_CFG_DEFAULT();
    i2s_reader.type = AUDIO_STREAM_READER;
    // raw_stream_cfg_t raw_reader = RAW_STREAM_CFG_DEFAULT();
    // raw_reader.type = AUDIO_STREAM_READER;

    //esp_audio_input_stream_add(player, raw_stream_init(&raw_reader));
    //esp_audio_input_stream_add(player, fatfs_stream_init(&fs_reader));
    esp_audio_input_stream_add(player, i2s_stream_init(&i2s_reader));
    // http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    // http_cfg.event_handle = _http_stream_event_handle;
    // http_cfg.type = AUDIO_STREAM_READER;
    // http_cfg.enable_playlist_parser = true;
    // audio_element_handle_t http_stream_reader = http_stream_init(&http_cfg);
    // esp_audio_input_stream_add(player, http_stream_reader);

    // Create writers and add to esp_audio
    // fatfs_stream_cfg_t fs_writer = FATFS_STREAM_CFG_DEFAULT();
    // fs_writer.type = AUDIO_STREAM_WRITER;

    i2s_stream_cfg_t i2s_writer = I2S_STREAM_CFG_DEFAULT();
    i2s_writer.type = AUDIO_STREAM_WRITER;

    // raw_stream_cfg_t raw_writer = RAW_STREAM_CFG_DEFAULT();
    // raw_writer.type = AUDIO_STREAM_WRITER;

    esp_audio_output_stream_add(player, i2s_stream_init(&i2s_writer));
    //esp_audio_output_stream_add(player, fatfs_stream_init(&fs_writer));
    //esp_audio_output_stream_add(player, raw_stream_init(&raw_writer));

    // Add decoders and encoders to esp_audio    
    wav_decoder_cfg_t  wav_dec_cfg  = DEFAULT_WAV_DECODER_CONFIG();
    esp_audio_codec_lib_add(player, AUDIO_CODEC_TYPE_DECODER, wav_decoder_init(&wav_dec_cfg));

    // Set default volume
    esp_audio_vol_set(player, 100);
    AUDIO_MEM_SHOW(TAG);
    ESP_LOGI(TAG, "esp_audio instance is:%p\r\n", player);

    // set configuration
    #ifdef  __WM8978_H 
    WM8978_Input_Cfg(0,0,0);

    

    uint16_t regval = wm8978_read_reg(1);
    ESP_LOGI(TAG, "wm8978 REG 1: 0X%X", regval);
    regval = wm8978_read_reg(2);
    ESP_LOGI(TAG, "wm8978 REG 2: 0X%X", regval);
    regval = wm8978_read_reg(3);
    ESP_LOGI(TAG, "wm8978 REG 3: 0X%X", regval);

    wm8978_sleep();
    #endif
}