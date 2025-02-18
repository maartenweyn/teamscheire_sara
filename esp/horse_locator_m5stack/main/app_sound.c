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
#include "app_config.h"


#include "nvs_flash.h"
#include "nvs.h"

#ifdef  __WM8978_H 
#include "wm8978.h"
#else
#include "es8388.h"
#endif

#define TAG "APP_SOUND:"

#define MAX_LETTERS 15
#define MEM_CHUNK_SIZE 20*1024

static char samples_data[MEM_CHUNK_SIZE];


#define STORAGE_NAMESPACE "sound"


static esp_audio_handle_t player;

typedef struct {
    char l;
    uint8_t pages;
} letter_page_info_t;

static letter_page_info_t letter_page_info[MAX_LETTERS] = {{' ', 0},};


static int8_t find_letter_info(char l) {
    for (int i = 0; i<MAX_LETTERS; i++) {
        if (letter_page_info[i].l == l) return i;
    }

    return -1;
}

static int8_t find_empty_letter_index() {
    for (int i = 0; i<MAX_LETTERS; i++) {
        if (letter_page_info[i].l == ' ') return i;
    }

    return -1;
}

int init_letter_storage() {
    nvs_handle my_handle;
    esp_err_t err;
    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    ESP_LOGI(TAG, "nvs_open %d", err);
    if (err != ESP_OK) return err;

    size_t required_size = sizeof(letter_page_info); 
    err = nvs_get_blob(my_handle, "letter_page", &letter_page_info, &required_size);
    if (err == ESP_OK && required_size != 0) {
        // Close
        nvs_close(my_handle);
        return ESP_OK;
    }


    for (int i = 0; i<MAX_LETTERS; i++) {
        letter_page_info[i].l = ' ';
        letter_page_info[i].pages = 0;
    }
     
    required_size = sizeof(letter_page_info); 
    err = nvs_set_blob(my_handle, "letter_page", &letter_page_info, required_size);
    ESP_LOGI(TAG, "nvs_set_blob letter_page_info %d bytes: %d", required_size, err);
    if (err != ESP_OK) return err;

    err = nvs_commit(my_handle);
    ESP_LOGI(TAG, "nvs_commit %d", err);
    if (err != ESP_OK) return err;

    nvs_close(my_handle);

    return ESP_OK;
}

int save_sound(char letter)
{
    nvs_handle my_handle;
    esp_err_t err;

    int8_t letter_index = find_letter_info(letter);
    if (letter_index != -1) {
        ESP_LOGI(TAG, "save_sound %c already saved on letter_index %d", letter, letter_index);
        return 1;
    }

    letter_index = find_empty_letter_index();
    if (letter_index == -1) {
        ESP_LOGI(TAG, "save_sound no emtpy letter_index");
        return 2;
    }

    ESP_LOGI(TAG, "save_sound on letter_index %d", letter_index);

    char filename[50];
    sprintf(filename, "/sdcard/audio/%c.raw", letter);
    ESP_LOGI(TAG, "file %s", filename);

    ESP_LOGI(TAG, "save_sound open file");
	FILE *f= fopen(filename, "r");
	if (f == NULL) {
        ESP_LOGE(TAG,"Failed to open file:%s",filename);
        return -1;
	}

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    ESP_LOGI(TAG, "nvs_open %d", err);
    if (err != ESP_OK) return err;

    char page_name[10];
	int rlen;
	//int res;
    int totalsize = 0;
    int pages = 0;
	//char* samples_data = malloc(MEM_CHUNK_SIZE);
	do {
		rlen=fread(samples_data,1,MEM_CHUNK_SIZE,f);
        totalsize += rlen;

        sprintf(page_name, "%c_%d", letter, pages);
        ESP_LOGI(TAG, "page_name %s", page_name);

        ESP_LOGI(TAG, "totalsize %d %d", totalsize, rlen);

        // Write
        err = nvs_set_blob(my_handle, page_name, samples_data, rlen);
        ESP_LOGI(TAG, "nvs_set_blob %s %d bytes: %d", page_name, rlen, err);
        if (err != ESP_OK) return err;

        pages++;
	} while (rlen == MEM_CHUNK_SIZE);

    letter_page_info[letter_index].l = letter;
    letter_page_info[letter_index].pages = pages;

    size_t required_size = sizeof(letter_page_info); 
    err = nvs_set_blob(my_handle, "letter_page", &letter_page_info, required_size);
    ESP_LOGI(TAG, "nvs_set_blob letter_page %d bytes: %d", required_size, err);
    if (err != ESP_OK) return err;

    err = nvs_commit(my_handle);
    ESP_LOGI(TAG, "nvs_commit %d", err);
    if (err != ESP_OK) return err;

    // Close
	//free(samples_data);
    nvs_close(my_handle);
    fclose(f);
	f=NULL;
    return ESP_OK;
}

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
    int totalsize = 0;
	//char* samples_data = malloc(MEM_CHUNK_SIZE);
    size_t bytes_written = 0;
    ESP_LOGI(TAG, "aplay_raw read file");
	do {
		rlen=fread(samples_data,1,MEM_CHUNK_SIZE,f);
        totalsize += rlen;
        //ESP_LOGD(TAG, "aplay_raw read %d bytes", rlen);
		//datalen-=rlen;
		//hal_i2s_write(0,samples_data,rlen,5000);
        //res = 
        i2s_write(0, samples_data, rlen, &bytes_written, 1000);

        //ESP_LOGD(TAG, "aplay_raw i2s write %d, %d bytes", res, bytes_written);
	} while(rlen==MEM_CHUNK_SIZE);
	fclose(f);
	//free(samples_data);
    ESP_LOGI(TAG, "aplay_raw i2s writen %d bytes", totalsize);
	f=NULL;
    //esp_audio_vol_set(player, 0);

    ESP_LOGI(TAG, "aplay_raw sleep");
    audio_mute(true);
}

int play_from_memory(char l) {
    int8_t letter_index = find_letter_info(l);
    if (letter_index == -1) {
        return 1;
    }

    nvs_handle my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) return err;

    //char* samples_data = malloc(MEM_CHUNK_SIZE);
    char page_name[10];

    ESP_LOGI(TAG, "play_from_memory wake");
    audio_mute(false);

    size_t bytes_written; 

    ESP_LOGI(TAG, "play_from_memory retreiving %d pages", letter_page_info[letter_index].pages);
    for (int i = 0; i < letter_page_info[letter_index].pages; i++) {
        size_t required_size = MEM_CHUNK_SIZE; 
        sprintf(page_name, "%c_%d", l, i);
        err = nvs_get_blob(my_handle, page_name, samples_data, &required_size);
        if (err != ESP_OK || required_size == 0) {
            ESP_LOGE(TAG, "nvs_get_blob %s with size %d, ret %d", page_name, required_size, err);
            break;
        }

        err = i2s_write(0, samples_data, required_size, &bytes_written, 1000);
    }

    //free(samples_data);
    ESP_LOGI(TAG, "play_from_memory sleep");
    audio_mute(true);

    // Close
    nvs_close(my_handle);
    return err;

}

void play_letter(char l) {
    int ret = play_from_memory(l);
    if (ret == ESP_OK) return;

    ESP_LOGI(TAG, "play_letter %c not loaded yet", l);

    ret = save_sound(l);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "play_letter %c loaded, now play", l);
        play_letter(l);
    } else {
        ESP_LOGI(TAG, "play_letter %c could not be loaded, now play from sd", l);
        char file_name[50];
        sprintf(file_name, "/sdcard/audio/%c.raw", l);
        ESP_LOGI(TAG, "file %s", file_name);
        aplay_raw(file_name);
    }
}


void setup_player(void)
{
    if (player ) {
        return ;
    }
    esp_audio_cfg_t cfg = {
        .in_stream_buf_size = 6 * 1024,
        .out_stream_buf_size = MEM_CHUNK_SIZE,//6 * 1024,
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

    i2s_stream_cfg_t i2s_writer = I2S_STREAM_CFG_DEFAULT();
    i2s_writer.type = AUDIO_STREAM_WRITER;


    esp_audio_output_stream_add(player, i2s_stream_init(&i2s_writer));

    // Add decoders and encoders to esp_audio    
    wav_decoder_cfg_t  wav_dec_cfg  = DEFAULT_WAV_DECODER_CONFIG();
    esp_audio_codec_lib_add(player, AUDIO_CODEC_TYPE_DECODER, wav_decoder_init(&wav_dec_cfg));

    // Set default volume
    esp_audio_vol_set(player, app_config.volume);
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

    audio_mute(true);
    #endif
}

void set_volume() {
    esp_audio_vol_set(player, app_config.volume);
}