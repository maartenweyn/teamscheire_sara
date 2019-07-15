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
nr_measurments = 20

real_node_positions = {
  1: [0, 0], 
  2: [20, 0], 
  3: [-1, 38], 
  4: [19, 41]}

# real_node_positions = {
#   1: [0, 0]}

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

test_letters = ["H", "K", "F", "M", "E", "B"]
colors = {
  1:  [0, 0, 1],
  2 : [0, 1, 0], 
  3 : [0.8, 0, 0], 
  4 : [0.5, 0.5, 0]}


calibrated_positions = {}


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
    

# colors = {}
# for key in real_node_positions:
#   color = list(np.random.choice(range(255), size=3)/255.0)
#   colors[key] = color

distances = {}
for key in real_node_positions:
  distances[key]  = {}

for test_letter in test_letters:
  
  for key in real_node_positions:
    #ax.text(real_node_positions[key][0], real_node_positions[key][1], key, fontsize=12, horizontalalignment='center', color='g')
    distances[key][test_letter] = math.sqrt((real_node_positions[key][0] - letters[test_letter][0]) ** 2 + (real_node_positions[key][1] - letters[test_letter][1]) ** 2)
    noise = np.mean(np.random.normal(0,noise_size,nr_measurments))
    distances[key][test_letter] = round((distances[key][test_letter] + noise), 2)

    circle = patches.Circle(letters[test_letter], distances[key][test_letter], facecolor='none', edgecolor=colors[key])
    ax.add_patch(circle)


# print(distances)
intersections = {}
intersections_filtered = {}

for achor in real_node_positions:
  print(achor)
  # for key in distances[achor]: 
  #   print (key, distances[achor][key])

  # all posible compbinates
  #https://d-nb.info/1056937106/34
  result_list = list(itertools.combinations(distances[achor].items(), 2))

  intersections[achor] = []
  intersections_filtered[achor] = []
  sum_inter = [0, 0]
  nr_intersections = 0

  for pair in result_list:
    # print (pair)

    letter1 = pair[0][0]
    dist1   = pair[0][1]
    letter2 = pair[1][0]
    dist2   = pair[1][1] 
    x1 = letters[letter1][0]
    y1 = letters[letter1][1]
    x2 = letters[letter2][0]
    y2 = letters[letter2][1]

    # p1 = (x1 - x2) / (y2 - y1)
    # p2 = ((dist1**2 - dist2**2) + (x2**2 - x1**2) + (y2**2 - x1**2)) / (2 * y2 - 2 * y1)
    # q1 = p1**2 + 1
    # q2 = 2 * p1 * (p2 - y1) - 2 * x1
    # q3 = (p2 - y1)**2 - dist1**2 + x1**2

    # x12 = [(-q2 - math.sqrt(q2**2 - 4 * q1 * q2))/(2*q1),  (-q2 + math.sqrt(q2**2 - 4 * q1 * q2))/(2*q1)]
    # y12 = [p1 * x12[0] + p2, p1 * x12[1] + p2]

    inter = intersectTwoCircles(x1, y1, dist1, x2, y2, dist2)

    # print (inter)

    if (inter):
      circle1 = patches.Circle(inter[0], 0.5, facecolor='none', edgecolor='black')
      circle2 = patches.Circle(inter[1], 0.5, facecolor='none', edgecolor='black')
      ax.add_patch(circle1)
      ax.add_patch(circle2)
      intersections[achor].append(inter)

      for i in inter:
        sum_inter[0] += i[0]
        sum_inter[1] += i[1]
        nr_intersections += 1

  avg_inter = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]

  # print ("intersections", intersections[achor])
  print ("temp", avg_inter)

  sum_inter = [0 , 0]
  nr_intersections = 0
  for i in intersections[achor]:
    d1 = (i[0][0] - avg_inter[0]) ** 2 + (i[0][1] - avg_inter[1]) ** 2 
    d2 = (i[1][0] - avg_inter[0]) ** 2 + (i[1][1] - avg_inter[1]) ** 2 

    if d1 < dist_thr :
      sum_inter[0] += i[0][0]
      sum_inter[1] += i[0][1]
      nr_intersections += 1
    if d2 < dist_thr :
      sum_inter[0] += i[1][0]
      sum_inter[1] += i[1][1]
      nr_intersections += 1

  # print ("Intersection")

  avg_inter2 = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]

  for i in intersections[achor]:
    d1 = (i[0][0] - avg_inter2[0]) ** 2 + (i[0][1] - avg_inter2[1]) ** 2 
    d2 = (i[1][0] - avg_inter2[0]) ** 2 + (i[1][1] - avg_inter2[1]) ** 2 

    if (d1 < d2):
      intersections_filtered[achor].append(i[0])
      circle1 = patches.Circle(i[0], 0.5, facecolor=colors[achor], edgecolor='black')
      circle2 = patches.Circle(i[1], 0.5, facecolor='none', edgecolor='black')
    else:
      intersections_filtered[achor].append(i[1])
      circle1 = patches.Circle(i[1], 0.5, facecolor=colors[achor], edgecolor='black')
      circle2 = patches.Circle(i[0], 0.5, facecolor='none', edgecolor='black')

    ax.add_patch(circle1)
    ax.add_patch(circle2)
  
  circle = patches.Circle(avg_inter, 0.5, facecolor=colors[achor], edgecolor='y')
  ax.add_patch(circle)
  circle = patches.Circle(avg_inter2, 0.5, facecolor=colors[achor], edgecolor='g')
  ax.add_patch(circle)

  # print ("Filtered")
  # print(intersections_filtered)

  sum_inter = [0 , 0]
  nr_intersections = 0
  for i in intersections_filtered[achor]:
    sum_inter[0] += i[0]
    sum_inter[1] += i[1]
    nr_intersections += 1

  avg_inter = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]
  # print ("filtered", intersections_filtered[achor])
  print ("final", avg_inter)

  calibrated_positions[achor] = avg_inter

  circle = patches.Circle(avg_inter, 0.5, facecolor=colors[achor], edgecolor='r')
  ax.add_patch(circle)

  error = math.sqrt((avg_inter[0] - real_node_positions[achor][0])**2 + (avg_inter[1] - real_node_positions[achor][1])**2)

  print ("error", error)
    
fig1.savefig('calibration.png', dpi=180, bbox_inches='tight')








