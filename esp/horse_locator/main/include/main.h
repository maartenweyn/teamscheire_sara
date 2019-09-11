#ifndef MAIN_H
#define MAIN_H



#define ALLOW_DELAY   5
#define MEAS_AVERAGE  4

#define USE_MEASUREMENT_THRESHOLD MEAS_AVERAGE * 6 //4 * 6 (can miss MEAS_AVERAGE cycles)

//SDCARD
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

//#define SKIP_SD_CARD

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