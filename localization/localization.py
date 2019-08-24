#!/usr/bin/env python

# Maarten Weyn
# maarten@wesdec.be

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.patches as patches
import math
import numpy as np
import itertools
import serial
import time
import sys, os

noise_size = 0.3
dist_thr = 20**2
step_size = 1
warn_threshold = 3**2
use_measurement_threshold = 6 * 4

real_node_positions = {
  1: [0.15, 4.80], 
  2: [0.15, 0], 
  3: [17.05, 4.50], 
  4: [17.05, 0.30]}

calib_node_positions = {
  1: [0.15, 4.80], 
  2: [0.15, 0], 
  3: [17.05, 4.50], 
  4: [17.05, 0.30]}

letters = {
  "X":  [1.24, 2.83]}

colors = {
  1:  [0, 0, 1],
  2 : [0, 1, 0], 
  3 : [0.8, 0, 0], 
  4 : [0.5, 0.5, 0]}

field = [22.00, 4.90]
field_size_marging  = 2

alpha = 0.9

ranges = [-1, -1, -1, -1, -1, -1]
filtered_ranges = [-1, -1, -1, -1, -1, -1]
counter = [100, 100, 100, 100, 100, 100]

positions_x = []
positions_y = []
all_ranges = []

port = serial.Serial("/dev/tty.usbserial-FT98HUH7", baudrate=500000, timeout=3.0)

# x1,y1 is the center of the first circle, with radius r1
# x2,y2 is the center of the second ricle, with radius r2
def intersectTwoCircles(x1,y1,r1, x2,y2,r2) :
  centerdx = x1 - x2
  centerdy = y1 - y2
  R = math.sqrt(centerdx * centerdx + centerdy * centerdy)
  # print (r1, r2, R)

  if (not (abs(r1 - r2) <= R and R <= r1 + r2)) : #// no intersection
    return [] # empty list of results

  # intersection(s) should exist

  R2 = R*R
  R4 = R2*R2
  a = (r1*r1 - r2*r2) / (2 * R2)
  r2r2 = (r1*r1 - r2*r2)
  c = math.sqrt(2 * (r1*r1 + r2*r2) / R2 - (r2r2 * r2r2) / R4 - 1)

  fx = (x1+x2) / 2 + a * (x2 - x1)
  gx = c * (y2 - y1) / 2
  ix1 = fx + gx
  ix2 = fx - gx

  fy = (y1+y2) / 2 + a * (y2 - y1)
  gy = c * (x1 - x2) / 2
  iy1 = fy + gy
  iy2 = fy - gy

  # note if gy == 0 and gx == 0 then the circles are tangent and there is only one solution
  # but that one solution will just be duplicated as the code is currently written
  return [[ix1, iy1], [ix2, iy2]]

def draw():

  fig1 = plt.figure()
  ax = fig1.add_subplot(211, aspect='equal')

  ax.set_xlim(-1, field[0]+1)
  ax.set_ylim(-1, field[1]+1)

  rect = patches.Rectangle([0, 0], field[0], field[1], fill=False)
  ax.add_patch(rect)

  for key in letters:
    ax.text(letters[key][0], letters[key][1], key, fontsize=10, horizontalalignment='center', verticalalignment='center', color='r')

  for key in real_node_positions:
    ax.text(real_node_positions[key][0], real_node_positions[key][1], key, fontsize=12, horizontalalignment='center', verticalalignment='center', color='b')
    circle = patches.Circle(real_node_positions[key], 0.5, facecolor='y')
    ax.add_patch(circle)

  ax2 = fig1.add_subplot(212, aspect='equal')

  d = np.zeros(len(positions_x))
  for i in range(len(positions_x)):  
    d[i] = int(math.sqrt((positions_x[i] - letters['X'][0])**2 + (positions_y[i] - letters['X'][1])**2) * 100)

  ax2.plot(d)

  counter = 0
  for r in range(len(all_ranges)):
    circles = []

    for i in range(6):
      if all_ranges[r][i] > 0:
        circle_range = patches.Circle(calib_node_positions[i+1], all_ranges[r][i], facecolor='none', edgecolor=colors[i+1])
        ax.add_patch(circle_range)
        circles.append(circle_range)

    circle = patches.Circle((positions_x[r], positions_y[r]), 0.2, facecolor='black')
    ax.add_patch(circle_range)
    circles.append(circle_range)

    fig1.savefig('img/localization_'+str(counter).zfill(3)+'.png', dpi=180, bbox_inches='tight')

    for circle in circles:
      circle.remove()

    counter += 1

  ax.plot(positions_x, positions_y)

  fig1.savefig('img/localization.png', dpi=180, bbox_inches='tight')
      
def locate():
  print(ranges, counter)

  intersections = []
  intersections_filtered = []
  sum_inter = [0, 0]
  nr_intersections = 0

  for i in range(6):
    if (counter[i] < use_measurement_threshold):
      for j in range(i+1, 6):
        if (counter[j] < use_measurement_threshold):
          inter = intersectTwoCircles(calib_node_positions[i+1][0], calib_node_positions[i+1][1], ranges[i], calib_node_positions[j+1][0], calib_node_positions[j+1][1], ranges[j])
          # print ("inter", i, j, inter)

          if (inter):

            selected = []
            for intersection in inter:
              if (intersection[0] >= -field_size_marging and intersection[0] <= field[0] + field_size_marging
                and intersection[1] >= -field_size_marging and intersection[1] <= field[1] + field_size_marging): 
                  selected.append(intersection)
                  sum_inter[0] += intersection[0]
                  sum_inter[1] += intersection[1]
                  nr_intersections += 1
                  # print ("intersections", intersection)
            
                
            if (len(selected) > 0):
              intersections.append(selected)

  if (nr_intersections == 0):
    return False
  
  print("nr_intersections", nr_intersections)
  avg_inter = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]
  print ("temp", avg_inter)

  # sum_inter = [0 , 0]
  # nr_intersections = 0
  # for i in intersections:
  #   d1 = (i[0][0] - avg_inter[0]) ** 2 + (i[0][1] - avg_inter[1]) ** 2 
  #   d2 = (i[1][0] - avg_inter[0]) ** 2 + (i[1][1] - avg_inter[1]) ** 2 

  #   if d1 < dist_thr :
  #     sum_inter[0] += i[0][0]
  #     sum_inter[1] += i[0][1]
  #     nr_intersections += 1
  #   if d2 < dist_thr :
  #     sum_inter[0] += i[1][0]
  #     sum_inter[1] += i[1][1]
  #     nr_intersections += 1

  # avg_inter2 = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]

  for i in intersections:
    if (len(i) > 1):
      d1 = (i[0][0] - avg_inter[0]) ** 2 + (i[0][1] - avg_inter[1]) ** 2 
      d2 = (i[1][0] - avg_inter[0]) ** 2 + (i[1][1] - avg_inter[1]) ** 2 

      if (d1 < d2):
        intersections_filtered.append(i[0])
      else:
        intersections_filtered.append(i[1])
    else:
      intersections_filtered.append(i[0])

  sum_inter = [0 , 0]
  nr_intersections = 0
  for i in intersections_filtered:
    sum_inter[0] += i[0]
    sum_inter[1] += i[1]
    nr_intersections += 1

  avg_inter = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]
  # print ("filtered", intersections_filtered[achor])
  print ("final", avg_inter)

  return avg_inter

def main():
  last_id = 100
  current_counter = 0

  fig = plt.figure()
  plt.ion()
  ax = fig.add_subplot(111, aspect='equal')

  ax.set_xlim(-1, field[0]+1)
  ax.set_ylim(-1, field[1]+1)

  rect = patches.Rectangle([0, 0], field[0], field[1], fill=False)
  ax.add_patch(rect)

  for key in letters:
    ax.text(letters[key][0], letters[key][1], key, fontsize=10, horizontalalignment='center', verticalalignment='center', color='r')

  for key in real_node_positions:
    ax.text(real_node_positions[key][0], real_node_positions[key][1], key, fontsize=12, horizontalalignment='center', verticalalignment='center', color='b')
    circle = patches.Circle(real_node_positions[key], 0.5, facecolor=colors[key])
    ax.add_patch(circle)

  
  plt.draw()

  try:
    while True:
        line = port.readline() 
        if (not line.startswith( b'[' )):
            #print("- ", data.strip())
            data = line.strip().split(b',')
            # print (data, len(data))

            if (len(data) == 3):
                try: 
                    id = int(data[0]) - 1
                    # print (data[0], id)
                    measurement = int(data[2]) / 1000.0
                    # print (data[2], range)
                except ValueError:
                    print("error", data)
                    continue

                ranges[id] = measurement
                counter[id] = 0
                for i in [x for x in range(6) if x != id]:
                    if counter[i] < 100:
                        counter[i] += 1
        
                if (id < last_id):
                    pos = locate()
                    if (pos):
                      positions_x.append(pos[0])
                      positions_y.append(pos[1])
                    else:
                      positions_x.append(-1)
                      positions_y.append(-1)

                    for i in range(6):
                      if (counter[i] > use_measurement_threshold):
                          ranges[i] = 0

                    all_ranges.append(ranges.copy())

                    circles = []

                    for i in range(6):
                      if ranges[i] > 0:
                        circle_range = patches.Circle(calib_node_positions[i+1], ranges[i], facecolor='none', edgecolor=colors[i+1])
                        ax.add_patch(circle_range)
                        circles.append(circle_range)

                    if (pos):
                      circle = patches.Circle((pos[0], pos[1]), 0.2, facecolor='black')
                      ax.add_patch(circle)
                      circles.append(circle)

                    # fig1.savefig('img/localization_'+str(counter).zfill(3)+'.png', dpi=180, bbox_inches='tight')

                    

                    plt.draw()
                    plt.pause(.000001)

                    for circle in circles:
                      circle.remove()

                last_id = id

  except (KeyboardInterrupt, SystemExit):
      port.close()
      print("interrrupted - closing")
  except Exception as e:
      exc_type, exc_obj, exc_tb = sys.exc_info()
      fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
      print("exception - closing: ", str(e), exc_type, fname, exc_tb.tb_lineno)

  print("saving images", len(all_ranges), len(positions_x))
  # draw()

if __name__ == "__main__":
  main()






