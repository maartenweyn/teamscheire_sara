#!/usr/bin/env python

# Maarten Weyn
# maarten@wesdec.be

import matplotlib.pyplot as plt
import matplotlib.patches as patches
import math
import numpy as np
import itertools

noise_size = 30
dist_thr = 2000**2
step_size = 1
warn_threshold = 300**2

field = [2100, 3700]
field_size_marging  = 500

# real_node_positions = {
#   1: [2100, 6300], 
#   2: [0, 6300], 
#   3: [0, 0], 
#   4: [2100, 0], 
#   5: [0, 3200]}

# calib_node_positions = {
#   1: [2100, 6300], 
#   2: [0, 6300], 
#   3: [0, 0], 
#   4: [2100, 0], 
#   5: [0, 3200]}

real_node_positions = {
  1: [0, 0],
  2: [1720, 0],
  3: [2264, 3564], 
  4: [423, 3493], 
  5: [2068, 1705], 
  6: [176, 1670]}

calib_node_positions = real_node_positions

# letters = {
#   "A":  [10, 0],
#   "K" : [0, 6], 
#   "E" : [0, 20], 
#   "H" : [0, 34], 
#   "C" : [10, 40], 
#   "M" : [20, 34], 
#   "B" : [20, 20], 
#   "F" : [20, 6], 
#   "D" : [10, 6], 
#   "X" : [10, 20], 
#   "G" : [10, 34]}

# letters = {
#   "A":  [1000, 000],
#   "K" : [0000, 600], 
#   "E" : [0000, 3000], 
#   "H" : [0000, 5400], 
#   "C" : [1000, 6000], 
#   "M" : [2000, 5400], 
#   "B" : [2000, 3000], 
#   "F" : [2000, 600], 
#   "D" : [1000, 600], 
#   "X" : [1000, 3000], 
#   "G" : [1000, 5400],
#   "R" : [2000, 4200],
#   "S" : [0000, 4200],
#   "P" : [2000, 1800],
#   "V" : [0000, 1800] }

letters = {
  "A":  [925, 000],
  "K" : [100, 500], 
  "E" : [200, 1850], 
  "H" : [350, 3150], 
  "C" : [1325, 3650], 
  "M" : [2200, 3150], 
  "B" : [2050, 1850], 
  "F" : [1900, 500], 
  "D" : [1000, 500], 
  "X" : [1125, 1850], 
  "G" : [1225, 3150]}

colors = {
  1:  [0, 0, 1],
  2 : [0, 1, 0], 
  3 : [0.8, 0, 0], 
  4 : [0.5, 0.5, 0], 
  5 : [0.5, 0.5, 0.5], 
 6 : [0.5, 0.5, 1.0]}


# x1,y1 is the center of the first circle, with radius r1
# x2,y2 is the center of the second ricle, with radius r2
def intersectTwoCircles(x1,y1,r1, x2,y2,r2) :
  centerdx = x1 - x2
  centerdy = y1 - y2
  R = math.sqrt(centerdx * centerdx + centerdy * centerdy)
  print ("x, y, r1, r2, R", centerdx, centerdy, r1, r2, R)

  if (not (abs(r1 - r2) <= R and R <= r1 + r2)) : #// no intersection
    return [] # empty list of results

  # intersection(s) should exist

  R2 = R*R
  a = (r1*r1 - r2*r2) / (2 * R2)
  r1r2 = (r1*r1 - r2*r2)
  r2r2 = r1r2 * r1r2
  c1 = (r1*r1 + r2*r2) / R2
  c2 = (r1r2 * r1r2) / (R2)
  c3 = c2 / R2
  ca = math.sqrt(2 * c1 - c3 - 1)
  c = math.sqrt(2 * (r1*r1 + r2*r2) / R2 - (r1r2 * r1r2) / (R2*R2) - 1)

  print("R2, a, r1r2, r2r2: ", R2, a, r1r2, r2r2)  
  print("c1, c2, c3, ca, c: ", c1, c2, c3, ca, c)


  fx = (x1+x2) / 2 + a * (x2 - x1)
  gx = c * (y2 - y1) / 2
  ix1 = fx + gx
  ix2 = fx - gx

  fy = (y1+y2) / 2 + a * (y2 - y1)
  gy = c * (x1 - x2) / 2
  iy1 = fy + gy
  iy2 = fy - gy

  print("fx, gx, fy, gy: ", fx, gx, fy, gy)

  print("ix1, ix2, iy1, iy2: ", ix1, ix2, iy1, iy2)


  # note if gy == 0 and gx == 0 then the circles are tangent and there is only one solution
  # but that one solution will just be duplicated as the code is currently written
  return [[ix1, iy1], [ix2, iy2]]

fig1 = plt.figure()
ax = fig1.add_subplot(111, aspect='equal')

ax.set_xlim(-2000, 4000)
ax.set_ylim(-2000, 6000)

rect = patches.Rectangle([0, 0], 2000, 4000, fill=False)
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

#print(full_path)
counter = 0
#ranges = [-1, -1, 2084, 76, -1, -1]
#ranges = [1345, 2479, 5006, 4694, 2429, -1] # r
#ranges = [532, 2087, -1, -1, 2947, -1] # m
#ranges = [902, 1160, 6038, 6089, 2938] # c
#ranges = [1739, 146, 3594, 3688, 2248, -1] # x
ranges = [1639, 102, 3558, 3614, 2171, 1692] # x
letterpos = "z"


estimations = []
  #print(pos)
  #circle_pos = patches.Circle((pos[0], pos[1]), 0.4, facecolor='black')
  #ax.add_patch(circle_pos)
distances = {}

circle_ranges = []

p1 = (-100, -100)
p2 = (100, 100)

firstmin = 100000
secondmin = 100000
thirdmin = 100000
firstind = -1
secondind = -1
thirdind = -1
for key in real_node_positions:
  if ranges[counter]  != -1 :
    #ax.text(real_node_positions[key][0], real_node_positions[onedkey][1], key, fontsize=12, horizontalalignment='center', color='g')
    distances[key] = ranges[counter]
    
    print("Range", key, ranges[counter])

    #ranges
    circle_range = patches.Circle(calib_node_positions[key],  ranges[counter], facecolor='none', edgecolor=colors[key])
    ax.add_patch(circle_range)
    circle_ranges.append(circle_range)

    # if ranges[counter] < firstmin:
    #   thirdmin = secondmin
    #   thirdind = secondind
    #   secondmin = firstmin
    #   secondind = firstind
    #   firstmin = ranges[counter]
    #   firstind = key
    # elif  ranges[counter] < secondmin:
    #   thirdmin = secondmin
    #   thirdind = secondind
    #   secondmin = ranges[counter]
    #   secondind = key
    # elif ranges[counter] < thirdmin:
    #   thirdmin = ranges[counter]
    #   thirdind = key


  counter += 1


# mindistances = {}
# if firstind != -1:
#   mindistances[firstind] = firstmin
# if secondind != -1:
#   mindistances[secondind] = secondmin
# if thirdind != -1:
#   mindistances[thirdind] = thirdmin

# print("mindistances", mindistances)

result_list = list(itertools.combinations(distances.items(), 2))

print("result_list", result_list)

intersections1 = []
intersections2 = []
intersections_filtered = []
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

          circle1 = patches.Circle(intersection, 50, facecolor='none', edgecolor='black')
          ax.add_patch(circle1)
          selected.append(intersection)
      else:
          print("not selected")
    print("selected #", len(selected))
    if (len(selected) > 1):
      intersections2.append(selected)
    elif (len(selected) > 0):
      intersections1.append(selected)

if nr_intersections > 0:
  avg_inter = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]
else:
  avg_inter = [ -1, -1]

# print ("intersections", intersections[achor])
print ("temp", avg_inter)
circle = patches.Circle(avg_inter, 100, facecolor='b', edgecolor='yellow')
ax.add_patch(circle)


if nr_intersections > 2:
  for i in intersections1:
    print ("intersection1", i[0])
    intersections_filtered.append(i[0])
    circle = patches.Circle(i[0], 50, facecolor='r', edgecolor='none')
    ax.add_patch(circle)

  for i in intersections2:
    print ("intersection2", i)
    d1 = (i[0][0] - avg_inter[0]) ** 2 + (i[0][1] - avg_inter[1]) ** 2 
    d2 = (i[1][0] - avg_inter[0]) ** 2 + (i[1][1] - avg_inter[1]) ** 2 

    print ("d1, d2", d1, d2)

    if (d1 < d2):
      print ("selected", i[0])
      intersections_filtered.append(i[0])
      circle1 = patches.Circle(i[0], 50, facecolor='r', edgecolor='none')
      circle2 = patches.Circle(i[1], 50, facecolor='none', edgecolor='black')
    else:
      print ("selected", i[1])
      intersections_filtered.append(i[1])
      circle1 = patches.Circle(i[1], 50, facecolor='r', edgecolor='none')
      circle2 = patches.Circle(i[0], 50, facecolor='none', edgecolor='black')

      ax.add_patch(circle1)
      ax.add_patch(circle2)

    



  
  # circle = patches.Circle(avg_inter, 0.5, facecolor='r', edgecolor='y')
  # ax.add_patch(circle)
  # circles.append(circle)



  # print ("Filtered")
  # print(intersections_filtered)

  sum_inter = [0 , 0]
  nr_intersections = 0
  for i in intersections_filtered:
    sum_inter[0] += i[0]
    sum_inter[1] += i[1]
    nr_intersections += 1
    print("sel", i)

  if nr_intersections > 0:
    avg_inter = [sum_inter[0] / nr_intersections, sum_inter[1] / nr_intersections]
  else:
    avg_inter = [ -1, -1]


# print ("filtered", intersections_filtered[achor])
print ("final", avg_inter)

circle_estimation = patches.Circle(avg_inter, 100, facecolor='green', edgecolor='green')
circle_estimation.set_alpha(0.5)
ax.add_patch(circle_estimation)

near_letter = 0
for letter in letters:
  d = (letters[letter][0] - avg_inter[0])**2 + (letters[letter][1] - avg_inter[1])**2
  if d < warn_threshold:
    near_letter = 1
    break

estimations.append([avg_inter[0], avg_inter[1], near_letter])





for pos in estimations:
  if not pos[2]:
    circle_pos = patches.Circle((pos[0], pos[1]), 0.3, facecolor='grey', edgecolor='none')
  else :
    circle_pos = patches.Circle((pos[0], pos[1]), 0.3, facecolor='red', edgecolor='none')
  ax.add_patch(circle_pos)

fig1.savefig('img/localization_range_' + letterpos + '.png', dpi=180, bbox_inches='tight')
