#!/usr/bin/env python

# Maarten Weyn
# maarten@wesdec.be

import matplotlib.pyplot as plt
import matplotlib.patches as patches
import math
import numpy as np
import itertools

noise_size = 0.3
dist_thr = 20**2
step_size = 1
warn_threshold = 3**2

real_node_positions = {
  1: [0, 0], 
  2: [20, 0], 
  3: [-1, 38], 
  4: [19, 41]}

calib_node_positions = {
  1: [-0.22, 0.01], 
  2: [20.11, 0.13], 
  3: [-1.24, 38.00], 
  4: [18.80, 41.04]}

letters = {
  "A":  [10, 0],
  "K" : [0, 6], 
  "E" : [0, 20], 
  "H" : [0, 34], 
  "C" : [10, 40], 
  "M" : [20, 34], 
  "B" : [20, 20], 
  "F" : [20, 6], 
  "D" : [10, 6], 
  "X" : [10, 20], 
  "G" : [10, 34]}

colors = {
  1:  [0, 0, 1],
  2 : [0, 1, 0], 
  3 : [0.8, 0, 0], 
  4 : [0.5, 0.5, 0]}


field = [20, 40]
field_size_marging  = 2


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

fig1 = plt.figure()
ax = fig1.add_subplot(111, aspect='equal')

ax.set_xlim(-20, 40)
ax.set_ylim(-20, 60)

rect = patches.Rectangle([0, 0], 20, 40, fill=False)
ax.add_patch(rect)

for key in letters:
  ax.text(letters[key][0], letters[key][1], key, fontsize=10, horizontalalignment='center', verticalalignment='center', color='r')

for key in real_node_positions:
  ax.text(real_node_positions[key][0], real_node_positions[key][1], key, fontsize=12, horizontalalignment='center', verticalalignment='center', color='b')
  circle = patches.Circle(real_node_positions[key], 0.5, facecolor='y')
  ax.add_patch(circle)
    
path = [[10, 20], #X
        [19, 20], #B
        [19, 34], #M
        [18, 38],
        [16, 39],
        [10, 39], #C
        [4, 39],
        [2, 38],
        [1, 34], #H
        [1, 20], #E
        [1, 6],  #K
        [2, 2], 
        [4, 1], 
        [10, 1], #A
        [16, 1], 
        [18, 2], 
        [19, 6], #F
        [19, 20], #B
        [18, 30],
        [14, 33],
        [10, 34], #G
        [6,30],
        [6, 24],
        [10, 20], #X
        [16,16],
        [16, 10],
        [10, 6] #D
        ]

# print (path)

pos = path[0]
full_path = [(pos[0], pos[1])]


# for key in path:
#   circle = patches.Circle(key, 0.3, facecolor='y')
#   ax.add_patch(circle)

for position in path[1:]:
  go = True
  while go:
    d = math.sqrt((position[0] - pos[0])**2 + (position[1] - pos[1])**2)
    
    if d <= step_size:
      # print (pos, key, d)
      pos = position
      go = False
    else:
      step = np.round(d / step_size)
      # print (pos, key, d, step)
      pos[0] += (position[0] - pos[0]) / step
      pos[1] += (position[1] - pos[1]) / step
      
    #print(pos)
    full_path.append((pos[0], pos[1]))


#print(full_path)
counter = 0
distances = {}


estimations = []

for pos in full_path:
  circles = []

  print(pos)
  circle_pos = patches.Circle((pos[0], pos[1]), 0.4, facecolor='black')
  ax.add_patch(circle_pos)
  circles.append(circle_pos)
  distances[counter] = {}



  intersections = {}
  intersections_filtered = {}

  print ("counter", counter)
  
  for key in real_node_positions:
    #ax.text(real_node_positions[key][0], real_node_positions[onedkey][1], key, fontsize=12, horizontalalignment='center', color='g')
    distances[counter][key] = math.sqrt((real_node_positions[key][0] - pos[0]) ** 2 + (real_node_positions[key][1] - pos[1]) ** 2)
    noise = np.mean(np.random.normal(0,noise_size,1))
    distances[counter][key] = round((distances[counter][key] + noise), 2)
    

    #ranges
    circle_range = patches.Circle(calib_node_positions[key], distances[counter][key], facecolor='none', edgecolor=colors[key])
    ax.add_patch(circle_range)
    circles.append(circle_range)

  result_list = list(itertools.combinations(distances[counter].items(), 2))

  print("result_list", result_list)

  intersections[counter] = []
  intersections_filtered[counter] = []
  sum_inter = [0, 0]
  nr_intersections = 0

  for pair in result_list:
    # print (pair)

    letter1 = pair[0][0]
    dist1   = pair[0][1]
    letter2 = pair[1][0]
    dist2   = pair[1][1] 
    x1 = calib_node_positions[letter1][0]
    y1 = calib_node_positions[letter1][1]
    x2 = calib_node_positions[letter2][0]
    y2 = calib_node_positions[letter2][1]

    inter = intersectTwoCircles(x1, y1, dist1, x2, y2, dist2)

    if (inter):
      selected = []
      
      for intersection in inter:
        print ("intersection", intersection)
        print ("x margin", -field_size_marging, field[0] + field_size_marging)
        print ("y margin", -field_size_marging, field[1] + field_size_marging)
        if (intersection[0] >= -field_size_marging and intersection[0] <= field[0] + field_size_marging
          and intersection[1] >= -field_size_marging and intersection[1] <= field[1] + field_size_marging): 
  
            print("selected")
            sum_inter[0] += intersection[0]
            sum_inter[1] += intersection[1]
            nr_intersections += 1

            circle1 = patches.Circle(intersection, 0.5, facecolor='none', edgecolor='black')
            ax.add_patch(circle1)
            circles.append(circle1)
            selected.append(intersection)

      print("selected", selected)
      if (len(selected) > 0):
        intersections[counter].append(selected)
          

  avg_inter = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]

  # print ("intersections", intersections[achor])
  print ("temp", avg_inter)

  # sum_inter = [0 , 0]
  # nr_intersections = 0
  # for i in intersections[counter]:
  #   print("intersection", i)
  #   d1 = (i[0] - avg_inter[0]) ** 2 + (i[1] - avg_inter[1]) ** 2 
  #   if d1 < dist_thr :
  #     sum_inter[0] += i[0][0]
  #     sum_inter[1] += i[0][1]
  #     nr_intersections += 1

  #   if len(i) > 1:
  #     d2 = (i[1][0] - avg_inter[0]) ** 2 + (i[1][1] - avg_inter[1]) ** 2 
  #     if d2 < dist_thr :
  #       sum_inter[0] += i[1][0]
  #       sum_inter[1] += i[1][1]
  #       nr_intersections += 1

  # print ("Intersection")

  # avg_inter2 = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]
  
  for i in intersections[counter]:
    print ("intersection", i)
    if (len(i) > 1): 
      d1 = (i[0][0] - avg_inter[0]) ** 2 + (i[0][1] - avg_inter[1]) ** 2 
      d2 = (i[1][0] - avg_inter[0]) ** 2 + (i[1][1] - avg_inter[1]) ** 2 

      print ("d1, d2", d1, d2)

      if (d1 < d2):
        intersections_filtered[counter].append(i[0])
        circle1 = patches.Circle(i[0], 0.5, facecolor='r', edgecolor='none')
        circle2 = patches.Circle(i[1], 0.5, facecolor='none', edgecolor='black')
      else:
        intersections_filtered[counter].append(i[1])
        circle1 = patches.Circle(i[1], 0.5, facecolor='r', edgecolor='none')
        circle2 = patches.Circle(i[0], 0.5, facecolor='none', edgecolor='black')

        ax.add_patch(circle1)
        ax.add_patch(circle2)
        circles.append(circle1)
        circles.append(circle2)
    else:
      intersections_filtered[counter].append(i[0])
      circle1 = patches.Circle(i[0], 0.5, facecolor='r', edgecolor='none')
      ax.add_patch(circle1)
      circles.append(circle1)
    

  
  # circle = patches.Circle(avg_inter, 0.5, facecolor='r', edgecolor='y')
  # ax.add_patch(circle)
  # circles.append(circle)
  circle = patches.Circle(avg_inter, 0.5, facecolor='b', edgecolor='black')
  ax.add_patch(circle)
  circles.append(circle)

  # print ("Filtered")
  # print(intersections_filtered)

  sum_inter = [0 , 0]
  nr_intersections = 0
  for i in intersections_filtered[counter]:
    sum_inter[0] += i[0]
    sum_inter[1] += i[1]
    nr_intersections += 1

  avg_inter = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]
  # print ("filtered", intersections_filtered[achor])
  print ("final", avg_inter)

  circle_estimation = patches.Circle(avg_inter, 0.5, facecolor='black')
  circle_estimation.set_alpha(0.5)
  ax.add_patch(circle_estimation)
  circles.append(circle_estimation)

  near_letter = 0
  for letter in letters:
    d = (letters[letter][0] - avg_inter[0])**2 + (letters[letter][1] - avg_inter[1])**2
    if d < warn_threshold:
      near_letter = 1
      break

  estimations.append([avg_inter[0], avg_inter[1], near_letter])

  fig1.savefig('img/localization_is_'+str(counter).zfill(3)+'.png', dpi=180, bbox_inches='tight')

  for circle in circles:
    circle.remove()

  
  counter += 1

  #print(distances)

for pos in full_path:
  circle_pos = patches.Circle((pos[0], pos[1]), 0.3, facecolor='black', edgecolor='none')
  ax.add_patch(circle_pos)

for pos in estimations:
  if not pos[2]:
    circle_pos = patches.Circle((pos[0], pos[1]), 0.3, facecolor='grey', edgecolor='none')
  else :
    circle_pos = patches.Circle((pos[0], pos[1]), 0.3, facecolor='red', edgecolor='none')
  ax.add_patch(circle_pos)

fig1.savefig('img/localization_is.png', dpi=180, bbox_inches='tight')








