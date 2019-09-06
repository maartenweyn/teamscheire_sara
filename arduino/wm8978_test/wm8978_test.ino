#include "wm8978.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "i2s.h"

const byte I2S_NUM = 0; 
const byte SD_CS = 13;
const int SAMPLE_RATE  = 48000;  

#define CCCC(c1, c2, c3, c4)    ((c4 << 24) | (c3 << 16) | (c2 << 8) | c1)


i2s_config_t i2s_config = {
     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
     .sample_rate = SAMPLE_RATE,
     .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
     .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
     .dma_buf_count = 3,
     .dma_buf_len = 1024,
     .use_apll = false
};
    
//updated I2S pins for tAudio
i2s_pin_config_t pin_config = {
    .bck_io_num = 33, //this is BCK pin
    .ws_io_num = 25, // this is LRCK pin
    .data_out_num = 26, // this is DATA output pin
    .data_in_num = 27   //
};



typedef enum headerState_e {
    HEADER_RIFF, HEADER_FMT, HEADER_DATA, DATA
} headerState_t;

typedef struct wavRiff_s {
    uint32_t chunkID;
    uint32_t chunkSize;
    uint32_t format;
} wavRiff_t;

typedef struct wavProperties_s {
    uint32_t chunkID;
    uint32_t chunkSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} wavProperties_t;


void setup() {
  
  Serial.begin(115200);
  Serial.println("Setup");

  SPI.begin(14, 2, 15, 13);

  while(1) {
    Serial.print("\nInitializing SD card...");
  
    // we'll use the initialization code from the utility libraries
    // since we're just testing if the card is working!
    if (!SD.begin(SD_CS)) {
        Serial.println("Card Mount Failed");
        delay(2000);
        continue;
    } else {
        if (!SD.begin(SD_CS)) {
            Serial.println("Card failed, or not present");
            delay(2000);
            continue;
        } else {
          Serial.println("Card card ok");
        }
        break;
    }
  }


  Wire.begin(19,18);
  WM8978_Init();
}

int i2s_write_sample_nb(uint8_t sample){
  return i2s_write_bytes((i2s_port_t)I2S_NUM, (const char *)&sample, sizeof(uint8_t), 100);
}

/* read 4 bytes of data from wav file */
int read4bytes(File file, uint32_t *chunkId){
  int n = file.read((uint8_t *)chunkId, sizeof(uint32_t));
  return n;
}

int readbyte(File file, uint8_t *chunkId){
  int n = file.read((uint8_t *)chunkId, sizeof(uint8_t));
  return n;
}

/* these are function to process wav file */
int readRiff(File file, wavRiff_t *wavRiff){
  int n = file.read((uint8_t *)wavRiff, sizeof(wavRiff_t));
  return n;
}
int readProps(File file, wavProperties_t *wavProps){
  int n = file.read((uint8_t *)wavProps, sizeof(wavProperties_t));
  return n;
}


void loop1() {

  
}

void loop() {
  // put your main code here, to run repeatedly:

    /* open wav file and process it */
  File wav = SD.open("/001K.WAV");
  headerState_t state = HEADER_RIFF;
  wavProperties_t wavProps;

  
  if (wav) {    
    int c = 0;
    int n;
    while (wav.available()) {
      switch(state){
        
        // HEADER RIFF
        case HEADER_RIFF:
          wavRiff_t wavRiff;
          n = readRiff(wav, &wavRiff);
          if(n == sizeof(wavRiff_t)){
            if(wavRiff.chunkID == CCCC('R', 'I', 'F', 'F') && wavRiff.format == CCCC('W', 'A', 'V', 'E')){
              state = HEADER_FMT;
              Serial.println("HEADER_RIFF");
            }
          }
          break;

        // HEADER PROPERTIES
        case HEADER_FMT:
          n = readProps(wav, &wavProps);
          if(n == sizeof(wavProperties_t)){
            state = HEADER_DATA;
          }
          break;

        // HEADER DATA
        case HEADER_DATA:
          uint32_t chunkId, chunkSize;
          n = read4bytes(wav, &chunkId);
          if(n == 4){
            if(chunkId == CCCC('d', 'a', 't', 'a')){
              Serial.println("HEADER_DATA");
            }
          }
          n = read4bytes(wav, &chunkSize);
          if(n == 4){
            Serial.println("prepare data");
            state = DATA;
            //initialize i2s with configurations above
            i2s_driver_install((i2s_port_t)I2S_NUM, &i2s_config, 0, NULL);
            i2s_set_pin((i2s_port_t)I2S_NUM, NULL);
            //set sample rates of i2s to sample rate of wav file
            i2s_set_sample_rates((i2s_port_t)I2S_NUM, wavProps.sampleRate); 
          }
          break; 
          
        /* MUSIC DATA */
        case DATA:
          uint8_t data; 
          n = readbyte(wav, &data);
          i2s_write_sample_nb(data); 
          break;
      }
    }
    Wire.endTransmission(); 
    wav.close();
  } else {  
    Serial.println("Cannot open wav file");
  }
}
