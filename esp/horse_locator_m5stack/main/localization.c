#include "localization.h"
#include "main.h"
#include "app_config.h"
#include "uwb_parser.h"
#include "app_sound.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_leds.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#define TAG "LOCALIZ: "

#define M_PI 3.14159265358979323846
#define RESAMPLE_PERC 0.95
#define DEFAULT_WEIGHT 0.00001


int meas_ranges[6] = {-1,-1,-1,-1,-1,-1};
//int avg_meas_ranges[6] = {-1,-1,-1,-1,-1,-1};
int meas_absence_counter[6] = {100,100,100,100,100,100};
int meas_counter[6] = {0,0,0,0,0,0};

static particle_t* particles;
static particle_t* new_particles;

bool receiving_ranges = false;


localization_result_t current_position;

int connected_anchors = 0;
int last_position_counter = 0;

int nearby_letter = -1;

static bool particles_init = false;
static uint32_t prev_measurement_ts = 0;

static void init_particles() {
  ESP_LOGD(TAG, "init_particles");

  srand(xTaskGetTickCount());

  if (!particles_init) { 
    particles = (particle_t*) malloc(sizeof(particle_t) * app_config.particle_filter.nr_of_particles);
    new_particles = (particle_t*) malloc(sizeof(particle_t) * app_config.particle_filter.nr_of_particles);
    particles_init = true;
  }

  for (int i = 0; i < app_config.particle_filter.nr_of_particles; i++) {
    particles[i].x =      rand() % app_config.field_size.x;
    particles[i].y =      rand() % app_config.field_size.x;
    particles[i].speed =  rand() % app_config.particle_filter.max_speed;
    particles[i].direction =  (((double)rand())/RAND_MAX) * 2 * M_PI;//float in range 0 to 2 PI
    particles[i].weight = 1.0 / app_config.particle_filter.nr_of_particles;

    ESP_LOGD(TAG, "init %d (%d, %d) (%d, %f) %f", i, particles[i].x, particles[i].y, particles[i].speed, particles[i].direction, particles[i].weight);


  }
}

//particle filter based
static void update_weights() {
  double weight_sum = 0.0;

  for (int i = 0; i < app_config.particle_filter.nr_of_particles; i++) {
    if ((particles[i].x < -app_config.field_size_margin) || 
      (particles[i].y < -app_config.field_size_margin) || 
      (particles[i].x > app_config.field_size.x + app_config.field_size_margin) ||
      (particles[i].y > app_config.field_size.y + app_config.field_size_margin)) {
        particles[i].weight = 0.0;
        continue;
      }

    particles[i].weight = 1.0;

    for (int j=0;j<6;j++) {
      if (meas_absence_counter[j] < USE_MEASUREMENT_THRESHOLD) {
        int dx = app_config.node_positions[j].x - particles[i].x;
        int dy = app_config.node_positions[j].y - particles[i].y;
        int dz = app_config.node_positions[j].z - app_config.sensor_height;
        double R = sqrt(dx * dx + dy * dy + dz * dz) - meas_ranges[j];
        float w = DEFAULT_WEIGHT + exp(-0.5 * (R*R) / app_config.particle_filter.UWB_std2);
        particles[i].weight *= w;

        ESP_LOGD(TAG, "(%d, %d) vs  %d (r=%d)->  R=%f (%f): w=%f, pw=%.20f", particles[i].x, particles[i].y, j, meas_ranges[j], sqrt(dx * dx + dy * dy + dz * dz), R, w, particles[i].weight);
      }
    }


    weight_sum += particles[i].weight;
  }

  if (weight_sum == 0.0) {
    ESP_LOGE(TAG, "UPDATE step, weights of all particles is zero!!!! Reinitializing");
    init_particles();
    return;
  }

  ESP_LOGD(TAG, "weight_sum %.20f", weight_sum);

  // Normalize
  for (int i = 0; i < app_config.particle_filter.nr_of_particles; i++) {
    particles[i].weight /= weight_sum;

    ESP_LOGD(TAG, "update %d (%d, %d) (%d, %f) %.20f", i, particles[i].x, particles[i].y, particles[i].speed, particles[i].direction, particles[i].weight);

  }

  // test
  weight_sum = 0.0;
  for (int i = 0; i < app_config.particle_filter.nr_of_particles; i++) {
    weight_sum += particles[i].weight;
  }

  ESP_LOGD(TAG, "W validation %.20f",  weight_sum);
   
   
}


static void resample() {
  double  cum_weight_selected = 0.0;
  double  avg_particles[4] = {0.0, };

  int cum_sum_index = 0;
  float cum_sum =  particles[cum_sum_index].weight;

  for (int i = 0; i < app_config.particle_filter.nr_of_particles; i++) {
    float position = ((((float)rand())/RAND_MAX) + i) / app_config.particle_filter.nr_of_particles;
    //ESP_LOGD(TAG, "position < cum_sum? %.20f < %.20f ", position, cum_sum);
    while (position >= cum_sum) {
      cum_sum_index+=1;
      cum_sum += particles[cum_sum_index].weight;
      //ESP_LOGD(TAG, "cum_sum %d : %.20f ", cum_sum_index, cum_sum);
    }
    

    memcpy((void*) &new_particles[i], (void*) &particles[cum_sum_index], sizeof(particle_t));  
    //ESP_LOGD(TAG, "select %d (%d, %d) ->  %d (%d, %d)", cum_sum_index, particles[cum_sum_index].x, particles[cum_sum_index].y, i, new_particles[i].x, new_particles[i].y);



    avg_particles[0] += particles[cum_sum_index].x * particles[cum_sum_index].weight;
    avg_particles[1] += particles[cum_sum_index].y * particles[cum_sum_index].weight;
    cum_weight_selected += particles[cum_sum_index].weight;
  }

  ESP_LOGD(TAG, "total weighted (%f, %f)", avg_particles[0], avg_particles[1]);

  avg_particles[0] /= cum_weight_selected;
  avg_particles[1] /= cum_weight_selected;

  ESP_LOGD(TAG, "total avg (%f, %f)", avg_particles[0], avg_particles[1]);


  // Calculate standard deviation
  for (int i = 0; i < app_config.particle_filter.nr_of_particles; i++) {
      avg_particles[2] += new_particles[i].weight * (new_particles[i].x - avg_particles[0]) * (new_particles[i].x - avg_particles[0]);
      avg_particles[3] += new_particles[i].weight * (new_particles[i].y - avg_particles[1]) *(new_particles[i].y - avg_particles[1]);
  
      ESP_LOGD(TAG, "resample %d (%d, %d) (%d, %f) %.20f", i, new_particles[i].x, new_particles[i].y, new_particles[i].speed, new_particles[i].direction, new_particles[i].weight);

  }

  avg_particles[2] /= cum_weight_selected;
  avg_particles[3] /= cum_weight_selected;
  current_position.std.x = (int) sqrt(avg_particles[2]);
  current_position.std.y  = (int) sqrt(avg_particles[3]);
  current_position.pos.x = (int) avg_particles[0];
  current_position.pos.y  = (int) avg_particles[1];

  if ((current_position.std.x < app_config.particle_filter.std_min_threshold) && (current_position.std.y < app_config.particle_filter.std_min_threshold))
  {
    current_position.is_valid = true;
  } else {
    current_position.is_valid = false;
  }

  particle_t* temp = particles;
  particles = new_particles;
  new_particles = temp;
}

static void predict(double delta_t_sec) {
  ESP_LOGD(TAG, "predict with delta %f sec", delta_t_sec);


  for (int i = 0; i < app_config.particle_filter.nr_of_particles; i++) {
    // speed is mostly constant only small variation in this case =- 50 cm / s
    particles[i].speed +=  ((rand() % 100) - 50) * delta_t_sec;
    if (particles[i].speed  < 0)
      particles[i].speed = 0;
    else if (particles[i].speed  > app_config.particle_filter.max_speed)
      particles[i].speed = app_config.particle_filter.max_speed;

    particles[i].direction +=  ((double)(rand()/RAND_MAX)*M_PI/2 - M_PI/4) * delta_t_sec; // max change of =- 45degrees / sec
        
    if (particles[i].direction  < 0)
      particles[i].direction += M_PI * 2;
    else if (particles[i].direction  > M_PI * 2)
      particles[i].direction -= M_PI * 2;


    particles[i].x += (int) (particles[i].speed  * cos(particles[i].direction) + (rand() % 40) - 20) * delta_t_sec; //should be gaussian but to computational    
    particles[i].y += (int) (particles[i].speed  * sin(particles[i].direction) + (rand() % 40) - 20) * delta_t_sec;
    

    ESP_LOGD(TAG, "predict %d (%d, %d) (%d, %f) %f", i, particles[i].x, particles[i].y, particles[i].speed, particles[i].direction, particles[i].weight);

  }
}

/// intersection based localization
static bool intersectTwoCircles(position_t p1, int r1, position_t p2, int r2, position_t* i1, position_t* i2) {
  int centerdx = p1.x - p2.x;
  int centerdy = p1.y - p2.y;
  
  int R = sqrt(centerdx * centerdx + centerdy * centerdy);
  ESP_LOGD(TAG, "x, y, r, r, R %d, %d, %d, %d, %d", centerdx, centerdy, r1, r2, R);

  if ((! ((abs(r1 - r2) <= R)  && (R <= r1 + r2))) || (R == 0))  {// no intersection
    return false;
  }


  //intersection(s) should exist

  uint32_t R2 = R*R;
  int r1r2 = (r1*r1 - r2*r2);
  double  a = r1r2 / (2 * (double) R2);

  double c1 = (r1*r1 + r2*r2) / (double) R2;
  double c2 = r1r2 / (double) R2;
  double c3 = c2 * c2;
  double c = sqrt((2 *  c1) - c3  - 1);

  ESP_LOGD(TAG, "R2, a, r1r2: %d, %f, %d", R2, a, r1r2);
  ESP_LOGD(TAG, "c1, c2, c3, c: %f, %f, %f, %f", c1, c2, c3, c);


  float fx = (p1.x + p2.x) / 2 + a * (p2.x - p1.x);
  float gx = c * (p2.y - p1.y) / 2;
  i1->x = (int) (fx + gx);
  i2->x = (int) (fx - gx);

  float fy = (p1.y+p2.y) / 2 + a * (p2.y - p1.y);
  float gy = c * (p1.x - p2.x) / 2;
  i1->y = (int) (fy + gy);
  i2->y = (int) (fy - gy);

  ESP_LOGD(TAG, "fx, gx, fy, gy: %f, %f, %f, %f", fx, gx, fy, gy);

  ESP_LOGD(TAG, "i1.x, i1.y, i2.x, i2.y: %d, %d, %d, %d", i1->x, i1->y, i2->x, i2->y);

//  # note if gy == 0 and gx == 0 then the circles are tangent and there is only one solution
//  # but that one solution will just be duplicated as the code is currently written
//  return [[ix1, iy1], [ix2, iy2]]
  return true;
}

bool processMeasurement_intersections() {
  static position_t intersections[30] = {0,};
  static int nr_of_intersections = 0;
  static int intersection_pointer = 0;
  static position_t sum_intersection;

  // Print ranges
  for (int i = 0; i < 6; i++)
  {
    printf("%d\t", meas_ranges[i]);
  }
  printf("\n");
  for (int i = 0; i < 6; i++)
  {
    printf("%d\t", meas_absence_counter[i]);
  }
  printf("\n");

  // Calculate intersections
  nr_of_intersections = 0;
  intersection_pointer = 0;
  sum_intersection.x = 0;
  sum_intersection.y = 0;
  
  for (int i = 0; i < 6; i++) {
   if (meas_absence_counter[i] < USE_MEASUREMENT_THRESHOLD) {
     for (int j = i + 1; j <= 6; j++) {
       if (meas_absence_counter[j] < USE_MEASUREMENT_THRESHOLD) {
          ESP_LOGD(TAG, "find intersection %d, %d:", i, j);
          position_t i1;
          position_t i2;
          if (intersectTwoCircles(app_config.node_positions[i], meas_ranges[i], app_config.node_positions[j], meas_ranges[j], &i1, &i2)) {
            ESP_LOGD(TAG, "intersection: (%d, %d), (%d, %d)", i1.x, i1.y, i2.x, i2.y);
            bool good_intersection = false;
            if (i1.x >= -app_config.field_size_margin && i1.x <= app_config.field_size.x + app_config.field_size_margin
              && i1.y >= -app_config.field_size_margin && i1.y <= app_config.field_size.y + app_config.field_size_margin) {
                good_intersection = true;
                intersections[intersection_pointer * 2].x = i1.x;
                intersections[intersection_pointer * 2].y = i1.y;
                sum_intersection.x += i1.x;
                sum_intersection.y += i1.y;
                nr_of_intersections++;
            } else {
               intersections[intersection_pointer * 2].x = -100;
               intersections[intersection_pointer * 2].y = -100;
            }
            if ((i2.x >= -app_config.field_size_margin && i2.x <= app_config.field_size.x + app_config.field_size_margin)
              && (i2.y >= -app_config.field_size_margin && i2.y <= app_config.field_size.y + app_config.field_size_margin)){
                good_intersection = true;
                intersections[1 + intersection_pointer * 2].x = i2.x;
                intersections[1 + intersection_pointer * 2].y = i2.y;
                sum_intersection.x += i2.x;
                sum_intersection.y += i2.y;
                nr_of_intersections++;
            } else {
               intersections[1 + intersection_pointer * 2].x = -100;
               intersections[1 + intersection_pointer * 2].y = -100;
            }

            ESP_LOGD(TAG, "nr_of_intersections: %d", nr_of_intersections);

            if (good_intersection) {
              intersection_pointer++;
            }
          } else {
            ESP_LOGI(TAG, "none");
          }
        }
      }
    }
  }


  if (nr_of_intersections == 0) {
    // no intersections
    //last_position_counter++;
    // find smallest ranges
    uint32_t min_range = UINT32_MAX;
    int min_index = -1;
    for (int i = 0; i < 6; i++) {
      if (meas_absence_counter[i] < USE_MEASUREMENT_THRESHOLD) {
        if (meas_ranges[i] < min_range)
        {
          min_range = meas_ranges[i];
          min_index = i;
        }
      }
    }

    ESP_LOGD(TAG, "min range: %d: %d", min_index, min_range);


    if (min_index > -1){
      if (min_range < app_config.nearby_threshold) {
        current_position.pos.x = app_config.node_positions[min_index].x;
        current_position.pos.y = app_config.node_positions[min_index].y;
      } else {
        return false;
      }
    } else {
      return false;
    }
  } else {
  
    position_t avg_intersection = {sum_intersection.x / nr_of_intersections, sum_intersection.y / nr_of_intersections, 0};
    sum_intersection.x = 0;
    sum_intersection.y = 0;

    ESP_LOGD(TAG, "avg_intersection: %d, %d", avg_intersection.x, avg_intersection.y);

    if (nr_of_intersections > 2 ) {
      // select the intersections closed to the current average, if there are 2
      for (int i = 0; i < intersection_pointer; i++) {
        position_t selected_position;
        if (intersections[i*2].x == -100) {
          selected_position.x = intersections[1 + i*2].x;
          selected_position.y = intersections[1 + i*2].y;
        } else if (intersections[1 + i*2].x == -100) {
          selected_position.x = intersections[i*2].x;
          selected_position.y = intersections[i*2].y;
        } else {
          int diffx1 = (intersections[i*2].x - avg_intersection.x);
          int diffy1 = (intersections[i*2].y - avg_intersection.y);
          int d1 = diffx1 * diffx1 + diffy1 * diffy1;
          int diffx2 = (intersections[1+i*2].x - avg_intersection.x);
          int diffy2 = (intersections[1+i*2].y - avg_intersection.y);
          int d2 = diffx2 * diffx2+ diffy2 * diffy2;
          if (d1 <= d2) {
            selected_position.x = intersections[i*2].x;
            selected_position.y = intersections[i*2].y;
          } else {
            selected_position.x = intersections[1 + i*2].x;
            selected_position.y = intersections[1 + i*2].y;
          }
        }
        intersections[i].x = selected_position.x;
        intersections[i].y = selected_position.y;
        sum_intersection.x += selected_position.x;
        sum_intersection.y += selected_position.y;
      }
      current_position.pos.x = sum_intersection.x / intersection_pointer;
      current_position.pos.y = sum_intersection.y / intersection_pointer;
    } else {
      current_position.pos.x = avg_intersection.x;
      current_position.pos.y = avg_intersection.y;
    }
  }

  ESP_LOGD(TAG, "current_position: %d, %d", current_position.pos.x, current_position.pos.y);

  last_position_counter = 0;

  nearby_letter = -1;
  for (int i = 0; i < NR_OF_LETTERS; i++) {
    int d = pow(app_config.letters[i].x - current_position.pos.x, 2) + pow(app_config.letters[i].y - current_position.pos.y, 2);
    if (d < app_config.nearby_threshold * app_config.nearby_threshold) {
      nearby_letter = i;
      break;
    }
  }
  
  return true;
}

bool processMeasurement () {
  // Print ranges
  for (int i = 0; i < 6; i++)
  {
    printf("%d\t", meas_ranges[i]);
  }
  printf("\n");
  for (int i = 0; i < 6; i++)
  {
    printf("%d\t", meas_absence_counter[i]);
  }
  printf("\n");

  uint32_t ts = xTaskGetTickCount();
  double diff = (ts - prev_measurement_ts) / 1000.0;

  predict(diff);

  update_weights();
  resample();


  ESP_LOGD(TAG, "current_position: (%d, %d) std (%d, %d), valid: %d, counter %d", current_position.pos.x, current_position.pos.y, current_position.std.x, current_position.std.y, current_position.is_valid, last_position_counter);

  if (current_position.is_valid) {
    last_position_counter = 0;

    nearby_letter = -1;
    for (int i = 0; i < NR_OF_LETTERS; i++) {
      int d = pow(app_config.letters[i].x - current_position.pos.x, 2) + pow(app_config.letters[i].y - current_position.pos.y, 2);
      if (d < app_config.nearby_threshold * app_config.nearby_threshold) {
        nearby_letter = i;
        break;
      }
    }
  }
  
  return true;
}

void initialize_localization_engine() {
  init_particles();

  prev_measurement_ts = xTaskGetTickCount();
}

void watch_position( void *pvParameters ){
  while(1) {
    if (receiving_ranges) {
      if ((last_position_counter < ALLOW_DELAY)) {
        ESP_LOGI(TAG, "current_position: (%d, %d) std (%d, %d), valid: %d, counter %d", current_position.pos.x, current_position.pos.y, current_position.std.x, current_position.std.y, current_position.is_valid, last_position_counter);

        if ((nearby_letter > -1)) {
          play_letter(app_config.letters[nearby_letter].letter);
          ESP_LOGI(TAG, "letter");
          leds_blink(0, 255, 0, 0, 50);
          //leds_setcolor(4, 100, 100, 100);

        } else {
          ESP_LOGI(TAG, "position %d %d, no letter", current_position.pos.x, current_position.pos.y);
          //leds_setcolor(4, 0, 100, 0);

        leds_blink(0, 255, 255, 0, 50);
        }
      } else {
        ESP_LOGI(TAG, "ranges, no position");
        //leds_setcolor(4, 0, 100, 100);
        leds_blink(0, 0, 255, 0, 50);
      }

      receiving_ranges = false;
    } else {
      ESP_LOGI(TAG, "No Ranges");
      leds_blink(255, 0, 0, 0, 50);
      //play_letter('x');
      //leds_setcolor(4, 100, 0, 0);
    }
    
    last_position_counter++;

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void uwb_test_range() {

  //int ranges[6] = {1639, 102, 3558, 3614, 2171, 1692};
  //int ranges[6] = {314, -1, 3965, -1, 1542, 2365};  
  //int ranges[6] = {1134, -1, -1, -1, 1865, -1};
  //int ranges[6] = {631, 102, 3910, 3517, 1764, 2214}; 
  int ranges[6] = {100, -1, -1, -1, -1, -1};  
  int counter[6] = {0,15,14,13,12,11};

  memcpy(meas_ranges, ranges, sizeof(ranges));
  memcpy(meas_absence_counter, counter, sizeof(ranges));


  uint32_t ts = xTaskGetTickCount();
  processMeasurement();

  ESP_LOGI(TAG, "PF precessing %d ms", xTaskGetTickCount() - ts);
  ESP_LOGI(TAG, "current_position: (%d, %d) std (%d, %d), valid: %d, counter %d", current_position.pos.x, current_position.pos.y, current_position.std.x, current_position.std.y, current_position.is_valid, last_position_counter);


  ts = xTaskGetTickCount();
  processMeasurement();  
  ESP_LOGI(TAG, "PF precessing %d ms", xTaskGetTickCount() - ts);
  ESP_LOGI(TAG, "current_position: (%d, %d) std (%d, %d), valid: %d, counter %d", current_position.pos.x, current_position.pos.y, current_position.std.x, current_position.std.y, current_position.is_valid, last_position_counter);

  ts = xTaskGetTickCount();
  processMeasurement();  
  ESP_LOGI(TAG, "PF precessing %d ms", xTaskGetTickCount() - ts);
  ESP_LOGI(TAG, "current_position: (%d, %d) std (%d, %d), valid: %d, counter %d", current_position.pos.x, current_position.pos.y, current_position.std.x, current_position.std.y, current_position.is_valid, last_position_counter);

}

void locator_task( void *pvParameters ){
  (void) pvParameters;
    
  xTaskCreatePinnedToCore(watch_position, "watch_position", 4096, NULL, 20, NULL, 1);


  //uwb_test_range();
  //bool got_position = uwb_parser_check_data();

  while(1) {
    //bool got_position = uwb_parser_check_data();
    uwb_parser_check_data();
    // vTaskSuspend(NULL);
  }

 }