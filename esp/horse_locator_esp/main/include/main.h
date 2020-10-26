#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>

#define ALLOW_DELAY   5
#define MEAS_AVERAGE  4

#define USE_MEASUREMENT_THRESHOLD 6 //MEAS_AVERAGE * 6 //4 * 6 (can miss MEAS_AVERAGE cycles)

//SDCARD
// #define PIN_NUM_MISO 2
// #define PIN_NUM_MOSI 15
// #define PIN_NUM_CLK  14
// #define PIN_NUM_CS   13

//#define SKIP_SD_CARD

// SOUND
#define SOUND_I2C_PORT  1
#define SOUND_I2C_SDA_PIN 21 //19
#define SOUND_I2C_SCL_PIN 22 //18

#define CONFIG_I2S_BCK_PIN 5
#define CONFIG_I2S_WS_PIN 13 
#define CONFIG_I2S_OUT_PIN 34 

#define NR_OF_LETTERS 15

typedef struct {
  int x;
  int y;
  int z;
} position_t;

typedef struct {
   position_t pos;
   position_t std;
   bool is_valid;
} localization_result_t;

typedef struct {
  char letter;
  int x;
  int y;
} letter_position_t;


extern int meas_ranges[];
extern int meas_counter[];
extern int meas_absence_counter[];

extern localization_result_t current_position;

#endif