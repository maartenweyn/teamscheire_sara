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

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#define TAG "LOCALIZ: "


int meas_ranges[6] = {-1,-1,-1,-1,-1,-1};
int avg_meas_ranges[6] = {-1,-1,-1,-1,-1,-1};
int meas_absence_counter[6] = {100,100,100,100,100,100};
int meas_counter[6] = {0,0,0,0,0,0};

bool receiving_ranges = false;


position_t current_position;

int connected_anchors = 0;
int last_position_counter = 0;

int nearby_letter = -1;


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
    printf("%d\t", avg_meas_ranges[i]);
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
          if (intersectTwoCircles(app_config.node_positions[i], avg_meas_ranges[i], app_config.node_positions[j], avg_meas_ranges[j], &i1, &i2)) {
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
        if (avg_meas_ranges[i] < min_range)
        {
          min_range = avg_meas_ranges[i];
          min_index = i;
        }
      }
    }

    ESP_LOGD(TAG, "min range: %d: %d", min_index, min_range);


    if (min_index > -1){
      if (min_range < app_config.nearby_threshold) {
        current_position.x = app_config.node_positions[min_index].x;
        current_position.y = app_config.node_positions[min_index].y;
      } else {
        return false;
      }
    } else {
      return false;
    }
  } else {
  
    position_t avg_intersection = {sum_intersection.x / nr_of_intersections, sum_intersection.y / nr_of_intersections};
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
      current_position.x = sum_intersection.x / intersection_pointer;
      current_position.y = sum_intersection.y / intersection_pointer;
    } else {
      current_position.x = avg_intersection.x;
      current_position.y = avg_intersection.y;
    }
  }

  ESP_LOGD(TAG, "current_position: %d, %d", current_position.x, current_position.y);

  last_position_counter = 0;

  nearby_letter = -1;
  for (int i = 0; i < NR_OF_LETTERS; i++) {
    int d = pow(app_config.letters[i].x - current_position.x, 2) + pow(app_config.letters[i].y - current_position.y, 2);
    if (d < app_config.nearby_threshold * app_config.nearby_threshold) {
      nearby_letter = i;
      break;
    }
  }
  
  return true;
}

bool processMeasurement () {
  
}

void watch_position( void *pvParameters ){
  while(1) {
    if (receiving_ranges) {
      if ((last_position_counter < ALLOW_DELAY)) {
        if ((nearby_letter > -1)) {
          play_letter(app_config.letters[nearby_letter].letter);
          ESP_LOGI(TAG, "letter");
          leds_blink(0, 255, 0, 0, 50);
          //leds_setcolor(4, 100, 100, 100);

        } else {
          ESP_LOGI(TAG, "position %d %d, no letter", current_position.x, current_position.y);
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