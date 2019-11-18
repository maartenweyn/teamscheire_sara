
# Maarten Weyn
# maarten@wesdec.be
# convert -delay 10 -quality 100 localization_pf_*.png movie_new01.mpg


import matplotlib.pyplot as plt
import matplotlib.patches as patches
import math
import numpy as np
import itertools
import random
from copy import deepcopy
import pandas as pd 
import csv

noise_size = 30
dist_thr = 2000**2
step_size = 1
warn_threshold = 300**2
meas_threshold = 3 * 6
DEFAULT_WEIGHT = 0.00001
RESAMPLE_PERC = 0.95

start_index = 15
end_index = -1

max_speed = 400 # 182 #m/s #https://www.sciencedirect.com/topics/agricultural-and-biological-sciences/dressage
std_speed = 100

UWB_std = 50
UWB_std2 = UWB_std**2

field = [2100, 3700]
field_size_marging  = 200

cmap = plt.cm.get_cmap("rainbow")
particle_patches = []

real_node_positions = {
  1: [0, 0, 100],
  2: [2060, 0, 100],
  3: [2210, 1730, 100], 
  4: [2060, 3540, 100], 
  5: [0, 3540, 100], 
  6: [-50, 1740, 100]}

calib_node_positions = real_node_positions

letters = {
  "A":  [1030, 0],
  "K" : [0, 531], 
  "E" : [0, 1770], 
  "H" : [0, 3009], 
  "C" : [1030, 3540], 
  "M" : [2060, 3009], 
  "B" : [2060, 1770], 
  "F" : [2060, 531], 
  "D" : [0, 0], 
  "X" : [1030, 1770], 
  "G" : [0, 0]}

colors = {
  1:  [0, 0, 1],
  2 : [0, 1, 0], 
  3 : [0.8, 0, 0], 
  4 : [0.5, 0.5, 0], 
  5 : [0.5, 0.5, 0.5], 
  6 : [0.5, 0.5, 1.0]}

fig1 = plt.figure()
ax = fig1.add_subplot(111, aspect='equal')

ax.set_xlim(-200, field[0] + 200)
ax.set_ylim(-200, field[1] + 200)

#cax = fig1.add_axes([0.1, 0.1, 0.03, 0.8])
#im = ax.imshow(np.array([[0,1]]), cmap=cmap)
#fig1.colorbar(im, cax=cax, orientation='vertical')

rect = patches.Rectangle([0, 0], field[0], field[1], fill=False)
ax.add_patch(rect)

for key in letters:
  ax.text(letters[key][0], letters[key][1], key, fontsize=10, horizontalalignment='center', verticalalignment='center', color='r')

for key in real_node_positions:
  ax.text(real_node_positions[key][0], real_node_positions[key][1], key, fontsize=12, horizontalalignment='center', verticalalignment='center', color='b')
  circle = patches.Circle(real_node_positions[key], 0.5, facecolor='y')
  ax.add_patch(circle)
    
circle_ranges = []

# typedef particle {
#   int16_t x;
#   int16_t x;
#   int16_t speed; ??cm/s
#   double direction; ?? radians
#   double w;
# }

particles = []
nr_of_particles = 500
particle_patches = []

def init_particles():

  global particles
  global partice_patches

  for i in range(nr_of_particles):
    particle = [random.randint(0, field[0]), random.randint(0, field[1]), random.randint(0, max_speed), random.uniform(0, 2*math.pi), 1.0/nr_of_particles]
    particles.append(particle)

    p = patches.Circle([particle[0], particle[1]], 5, color=cmap(particle[4]))
    ax.add_patch(p)
    particle_patches.append(p)

  fig1.savefig('img/localization_pf_0000.png', dpi=300, bbox_inches='tight')

  # print("particles", particles)

  for p in particle_patches:
    p.remove()

def set_particles():
  global particles
  global particle_patches

  particle = [1477, 28, random.randint(0, max_speed), random.uniform(0, 2*math.pi), 1.0/nr_of_particles]
  particles.append(particle)

  p = patches.Circle([particle[0], particle[1]], 5, color=cmap(particle[4]))
  ax.add_patch(p)
  particle_patches.append(p)

  particle = [1700, 28, random.randint(0, max_speed), random.uniform(0, 2*math.pi), 1.0/nr_of_particles]
  particles.append(particle)

  p = patches.Circle([particle[0], particle[1]], 5, color=cmap(particle[4]))
  ax.add_patch(p)
  particle_patches.append(p)

  fig1.savefig('img/localization_pf_0000.png', dpi=300, bbox_inches='tight')

  # print("particles", particles)

  for p in particle_patches:
    p.remove()

def update_weights(ranges):
  global particles
  weights_sum = 0.0

  # Update weights based on measurements
  for particle in particles:

    if (particle[0] < -field_size_marging) or (particle[1] < -field_size_marging) or (particle[0] > field[0] + field_size_marging) or (particle[1] >  field[1] + field_size_marging): 
        particle[4] = 0
        #print('out', particle)
        continue

    particle[4] = 1.0
    for i in range(6):
      if (ranges[i] >= 0) and (ranges[i+6] < meas_threshold):

        dx = real_node_positions[i+1][0] - particle[0]
        dy = real_node_positions[i+1][1] - particle[1]
        dz = real_node_positions[i+1][2] - 248
        R = math.sqrt(dx * dx + dy * dy + dz * dz)

        num = DEFAULT_WEIGHT + math.exp(-0.5 * (R-ranges[i])**2 / UWB_std2)
        #double denom = 2 * M_PI * std_x
        particle[4] *= num#/denom;

       # print ("x, y, id, r, R, w, pw", particle[0], particle[1], i + 1, ranges[i], R, num, particle[4])
    
    weights_sum += particle[4]

  #normalize weights
  for particle in particles:
    particle[4] /= weights_sum

    #print(particle)

def resample(particles):

  new_particles = []
  cum_weight_seleted = 0.0
  avg_particles = [0.0, 0.0, 0.0, 0.0]

  cum_sum_index = 0
  cum_sum =  particles[cum_sum_index][4]
  i = 0
  resample_number = math.floor(nr_of_particles * RESAMPLE_PERC)
  for  i in range(resample_number):
    position = (random.random() + i) / (resample_number)
    #print("pos < cumsum?: cum_sum_index", position, cum_sum, cum_sum_index)
    while position >= cum_sum:
      cum_sum_index+=1
      cum_sum += particles[cum_sum_index][4]

      #print("cumsum %d: %.20f", cum_sum_index, cum_sum)
    
    new_particles.append(deepcopy(particles[cum_sum_index]))  
    avg_particles[0] += particles[cum_sum_index][0] * particles[cum_sum_index][4]
    avg_particles[1] += particles[cum_sum_index][1] * particles[cum_sum_index][4]
    cum_weight_seleted += particles[cum_sum_index][4]
    #print(particles[cum_sum_index])

  print("total %.20f %.20f", avg_particles[0], avg_particles[1])


  avg_particles[0] /= cum_weight_seleted
  avg_particles[1] /= cum_weight_seleted

  print("avg %.20f %.20f", avg_particles[0], avg_particles[1])

  std_x = 0.0
  std_y = 0.0
  for i in range(resample_number):
      std_x += new_particles[i][4] * (new_particles[i][0] - avg_particles[0]) ** 2
      std_y += new_particles[i][4] * (new_particles[i][1] - avg_particles[1]) ** 2
  std_x /= cum_weight_seleted
  std_y /= cum_weight_seleted
  avg_particles[2] = math.sqrt(std_x)
  avg_particles[3] = math.sqrt(std_y)

  for i in range(nr_of_particles - resample_number):
    particle = [random.randint(0, field[0]), random.randint(0, field[1]), random.randint(0, max_speed), random.uniform(0, 2*math.pi), 0]
    new_particles.append(particle)

  return new_particles, avg_particles

def predict(delta_t):
  global particles
  global particle_patches
  global ax
  global fig1

  for particle in particles:
    #oldparticle = deepcopy(particle)

    particle[2] += math.floor((random.randint(0, 100) - 50) * delta_t)
    if (particle[2] < 0):
      particle[2] = 0
    elif particle[2] > max_speed:
      particle[2] = max_speed

    particle[3] += (random.uniform(0, math.pi) - math.pi/2) * delta_t
    if (particle[3] < 0):
      particle[3] += math.pi * 2
    elif particle[3] > math.pi * 2:
      particle[3] -= math.pi * 2

    #particle[0] += (math.floor(particle[2] *  math.cos(particle[3])) * delta_t) #+ random.randint(0, 100) - 50)) * delta_t #should be gaussian but to computational
    #particle[1] += (math.floor(particle[2] *  math.sin(particle[3])) * delta_t) # + random.randint(0, 100) - 50)) * delta_t

    particle[0] += math.floor((particle[2] *  math.cos(particle[3])  * delta_t) + (random.randint(0, 100) - 50))
    particle[1] += math.floor((particle[2] *  math.sin(particle[3])  * delta_t) + (random.randint(0, 100) - 50))
    
    p = patches.Circle([particle[0], particle[1]], 5, color='grey')
    ax.add_patch(p)
    particle_patches.append(p)

    #print(oldparticle, " -> " , particle)

def plot_particles(suffix):
  global particle_patches
  global ax
  global fig1

  max_particle_w = 0.0
  for particle in particles:
    if (particle[4] > max_particle_w):
      max_particle_w = particle[4] 
  
  print("max_particle_w", max_particle_w)

  for particle in particles:
    p = patches.Circle([particle[0], particle[1]], 5, color=cmap(particle[4] / max_particle_w))
    ax.add_patch(p)
    particle_patches.append(p)

  print("Saving img ", suffix)

  fig1.savefig('img/localization_pf_' + suffix + '.png', dpi=300, bbox_inches='tight')

  for p in particle_patches:
    p.set_visible(False)

##INIT
init_particles()
#set_particles()

row_index = 1

data = pd.read_csv('data/all.CSV', sep='\t', header=None)
print (data)

if end_index == -1:
  end_index = data.shape[0]

for index, row in data.iterrows(): 
  print("index ", index) 
  if index < start_index: 
    continue
  if index > end_index: 
    break

  for i in range(6):
    if (row.iloc[i]  != -1) and (row.iloc[i+6] < meas_threshold):    
      #print("Range", i+1, row.iloc[i])

      #ranges
      circle_range = patches.Circle(real_node_positions[i+1],  row.iloc[i], facecolor='none', edgecolor=colors[i+1], linewidth=0.5)
      ax.add_patch(circle_range)
      circle_ranges.append(circle_range)

      min_range = row.iloc[i] - UWB_std
      if (min_range < 0):
          min_range - 5
      max_range = row.iloc[i] + UWB_std

      circle_range = patches.Circle(real_node_positions[i+1],  min_range, facecolor='none', edgecolor=colors[i+1], linestyle='-.', linewidth=0.5)
      ax.add_patch(circle_range)
      circle_ranges.append(circle_range)

      circle_range = patches.Circle(real_node_positions[i+1],  max_range, facecolor='none', edgecolor=colors[i+1], linestyle='-.', linewidth=0.5)
      ax.add_patch(circle_range)
      circle_ranges.append(circle_range)

  #UPDATE
  update_weights(row.iloc[0:12])
  #plot_particles("01")

  print("RESAMPLE")
  particles, position = resample(particles)
  print("position: ", position)

  circle_estimation = patches.Circle(position, 100, facecolor='green', edgecolor='green')
  circle_estimation.set_alpha(0.5)
  ax.add_patch(circle_estimation)
  plot_particles(str(index).zfill(4))
  print("POSITION", position)
  circle_estimation.remove()

  for c in circle_ranges[:]:
    c.set_visible(False)


  print("PREDICT")
  predict(0.1)

  #row_index = row_index + 1
