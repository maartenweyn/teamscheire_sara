#define USE_MEASUREMENT_THRESHOLD 24 //4 * 6 (can miss 3 cycles)

#define NEARBY_THRESHOLD 90000 // 3.00 m * 3.00 m

#define FIELD_SIZE_X      2200
#define FIELD_SIZE_Y      490
#define FIELD_SIZE_MARGIN 200


#define CONFIG_FILE "/sdcard/config.cfg"


typedef struct {
  int x;
  int y;
} position_t;


extern int meas_ranges[];
extern int meas_counter[];

extern position_t current_position;