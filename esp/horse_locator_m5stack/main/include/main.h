#ifndef MAIN_H
#define MAIN_H



#define ALLOW_DELAY   5
#define MEAS_AVERAGE  4

#define USE_MEASUREMENT_THRESHOLD MEAS_AVERAGE * 6 //4 * 6 (can miss MEAS_AVERAGE cycles)

//SDCARD
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   4

//#define SKIP_SD_CARD

// SOUND
#define SOUND_I2C_PORT  1
#define SOUND_I2C_SDA_PIN 21 //19
#define SOUND_I2C_SCL_PIN 22 //18

#define CONFIG_I2S_BCK_PIN 5
#define CONFIG_I2S_WS_PIN 13 
#define CONFIG_I2S_OUT_PIN 34 

#define NR_OF_LETTERS 15


#define CONFIG_FILE "/sdcard/config.cfg"


typedef struct {
  int x;
  int y;
} position_t;

typedef struct {
  char letter;
  int x;
  int y;
} letter_position_t;


extern int meas_ranges[];
extern int meas_counter[];
extern int meas_absence_counter[];

extern position_t current_position;

#endif