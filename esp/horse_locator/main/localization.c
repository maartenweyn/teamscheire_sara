#include "localization.h"
#include "main.h"
#include "app_config.h"

#include "esp_log.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define TAG "LOCALIZ: "

letter_position_t letters[NR_OF_LETTERS] = {
  {'A', 100, 0},
  //{'A', 1000, 0},
  {'K', 0, 600},
  {'E', 0, 2000}, 
  {'H', 0, 3400}, 
  {'C', 1000, 4000}, 
  {'M', 2000, 3400}, 
  {'B', 2000, 2000}, 
  {'F', 2000, 600}, 
  {'D', 1000, 600}, 
  {'X', 1000, 2000}, 
  {'G', 1000, 3400}
};


// calibration_ranges_t calibration_data[] = {
//     {'H', {0,0,0,0,0,0}, 0},
//     {'K', {0,0,0,0,0,0}, 0},
//     {'F', {0,0,0,0,0,0}, 0},
//     {'M', {0,0,0,0,0,0}, 0},
//     {'E', {0,0,0,0,0,0}, 0},
//     {'B', {0,0,0,0,0,0}, 0}};

//= {
//  {015, 480},
//  {015, 000},
//  {1705, 450},
//  {1705, 030},
//  {0, 0},
//  {0, 0}};

position_t current_position;

int connected_anchors = 0;
int last_position_counter = 0;

int nearby_letter = -1;


static bool intersectTwoCircles(position_t p1, int r1, position_t p2, int r2, position_t* i1, position_t* i2) {
  int centerdx = p1.x - p2.x;
  int centerdy = p1.y - p2.y;
  
  int R = sqrt(centerdx * centerdx + centerdy * centerdy);
  ESP_LOGI(TAG, "r, r, R %d, %d, %d\n", r1, r2, R);

  if ((! ((abs(r1 - r2) <= R)  && (R <= r1 + r2))) || (R == 0))  {// no intersection
    return false;
  }


  //intersection(s) should exist

  int R2 = R*R;
  int R4 = R2*R2;
  float  a = (r1*r1 - r2*r2) / (2 * R2);
  int r2r2 = (r1*r1 - r2*r2);
  int c = sqrt(2 * (r1*r1 + r2*r2) / R2 - (r2r2 * r2r2) / R4 - 1);

  int fx = (p1.x + p2.x) / 2 + a * (p2.x - p1.x);
  int gx = c * (p2.y - p1.y) / 2;
  i1->x = fx + gx;
  i2->x = fx - gx;

  int fy = (p1.y+p2.y) / 2 + a * (p2.y - p1.y);
  int gy = c * (p1.x - p2.x) / 2;
  i1->y = fy + gy;
  i2->y = fy - gy;

  ESP_LOGI(TAG, "a, c, fx, gx, fy, gy: %f, %d, %d, %d, %d, %d\n", a, c, fx, gx, fy, gy);

//  # note if gy == 0 and gx == 0 then the circles are tangent and there is only one solution
//  # but that one solution will just be duplicated as the code is currently written
//  return [[ix1, iy1], [ix2, iy2]]
  return true;
}

bool processMeasurement() {
  static position_t intersections[30] = {0,};
  static int nr_of_intersections = 0;
  static int intersection_pointer = 0;
  static position_t sum_intersection;

  
  // Visual Feedback
  //led_mode != led_mode;
  //digitalWrite(LED, led_mode);

  // Print ranges
  for (int i = 0; i < 6; i++)
  {
    printf("%d\t", meas_ranges[i]);
  }
  printf("\n");
  for (int i = 0; i < 6; i++)
  {
    printf("%d\t", meas_counter[i]);
  }
  printf("\n");


  // Calculate intersections
  nr_of_intersections = 0;
  intersection_pointer = 0;
  sum_intersection.x = 0;
  sum_intersection.y = 0;
  
  for (int i = 0; i < 5; i++) {
    if (meas_counter[i] < USE_MEASUREMENT_THRESHOLD) {
      for (int j = i + 1; j < 6; j++) {
        if (meas_counter[j] < USE_MEASUREMENT_THRESHOLD) {
          ESP_LOGI(TAG, "find intersection %d, %d:", i, j);
          position_t i1;
          position_t i2;
          if (intersectTwoCircles(app_config.node_positions[i], meas_ranges[i], app_config.node_positions[j], meas_ranges[j], &i1, &i2)) {
            ESP_LOGI(TAG, "-> (%d, %d), (%d, %d)", i1.x, i1.y, i2.x, i2.y);
            bool good_intersection = false;
            if (i1.x >= -FIELD_SIZE_MARGIN && i1.x <= FIELD_SIZE_X + FIELD_SIZE_MARGIN
              && i1.y >= -FIELD_SIZE_MARGIN && i1.y <= FIELD_SIZE_Y + FIELD_SIZE_MARGIN) {
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
            if ((i2.x >= -FIELD_SIZE_MARGIN && i2.x <= FIELD_SIZE_X + FIELD_SIZE_MARGIN)
              && (i2.y >= -FIELD_SIZE_MARGIN && i2.y <= FIELD_SIZE_Y + FIELD_SIZE_MARGIN)){
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
    last_position_counter++;
    return false;
  }
  
  position_t avg_intersection = {sum_intersection.x / nr_of_intersections, sum_intersection.y / nr_of_intersections};
  sum_intersection.x = 0;
  sum_intersection.y = 0;

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

  last_position_counter = 0;

  nearby_letter = -1;
  for (int i = 0; i < NR_OF_LETTERS; i++) {
    int d = pow(letters[i].x - current_position.x, 2) + pow(letters[i].y - current_position.y, 2);
    if (d < app_config.nearby_threshold * app_config.nearby_threshold) {
      nearby_letter = i;
      break;
    }
  }

  
  return true;
}


void setCalibration(char letter) {
  
}