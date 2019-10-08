
# Maarten Weyn
# maarten@wesdec.be

import matplotlib.pyplot as plt
import matplotlib.patches as patches
import math
import numpy as np
import itertools
import random
from copy import deepcopy

noise_size = 30
dist_thr = 2000**2
step_size = 1
warn_threshold = 300**2

max_speed = 200 # 182 #m/s #https://www.sciencedirect.com/topics/agricultural-and-biological-sciences/dressage
std_speed = 50

UWB_std2 = 50**2

field = [2100, 3700]
field_size_marging  = 200

cmap = plt.cm.get_cmap("rainbow")

real_node_positions = {
  1: [0, 0],
  2: [1720, 0],
  3: [2264, 3564], 
  4: [423, 3493], 
  5: [2068, 1705], 
  6: [176, 1670]}

calib_node_positions = real_node_positions

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


fig1 = plt.figure()
ax = fig1.add_subplot(111, aspect='equal')

ax.set_xlim(-200, field[0] + 200)
ax.set_ylim(-200, field[1] + 200)


cax = fig1.add_axes([0.1, 0.1, 0.03, 0.8])
im = ax.imshow(np.array([[0,1]]), cmap=cmap)
fig1.colorbar(im, cax=cax, orientation='vertical')

rect = patches.Rectangle([0, 0], field[0], field[1], fill=False)
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
#ranges = [1639, 102, 3558, 3614, 2171, 1692] # ('POSITION', [1551.76292120504, 64.32657013047535, 18.86662257100413, 21.16800102128328])
#ranges = [314, -1, 3965, -1, 1542, 2365] # -> ('POSITION', [1306.951920952366, 2057.5606398480504, 861.9465468528498, 1477.9641095034224])
ranges = [1134, -1, -1, -1, 1865, -1] #('POSITION', [289.031039843037, 1061.864161891347, 32.11865456078246, 14.433873809155772])
ranges = [631, -1, 3910, 3517, 1764, 2214]

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

#init particle filter

def init_particles():

  global particles

  for i in range(nr_of_particles):
    particle = [random.randint(0, field[0]), random.randint(0, field[1]), random.randint(0, max_speed), random.uniform(0, 2*math.pi), 1.0/nr_of_particles]
    particles.append(particle)

    p = patches.Circle([particle[0], particle[1]], 5, color=cmap(particle[4]))
    ax.add_patch(p)
    particle_patches.append(p)

  fig1.savefig('img/localization_pf_00.png', dpi=180, bbox_inches='tight')

  # print("particles", particles)

  for p in particle_patches:
    p.remove()

def set_particles():
  global particles

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

  fig1.savefig('img/localization_pf_00.png', dpi=180, bbox_inches='tight')

  # print("particles", particles)

  for p in particle_patches:
    p.remove()

def update_weights():
  global particles
  weights_sum = 0.0

  # Update weights based on measurements
  for particle in particles:
    particle[4] = 1.0

    if (particle[0] < -field_size_marging) or (particle[1] < -field_size_marging) or (particle[0] > field[0] + field_size_marging) or (particle[1] >  field[1] + field_size_marging): 
        particle[4] = 0
        print('out', particle)
        continue

    for i in range(6):
      if (ranges[i] >= 0):

        dx = real_node_positions[i+1][0] - particle[0]
        dy = real_node_positions[i+1][1] - particle[1]
        R = math.sqrt(dx * dx + dy * dy)

        num = 0.001 + math.exp(-0.5 * (R-ranges[i])**2 / UWB_std2)
        #double denom = 2 * M_PI * std_x
        particle[4] *= num#/denom;

        print ("x, y, id, r, R, w, pw", particle[0], particle[1], i + 1, ranges[i], R, num, particle[4])
    
    weights_sum += particle[4]


  #normalize weights
  for particle in particles:
    particle[4] /= weights_sum

    print(particle)


def resample(particles):

  new_particles = []
  cum_weight_seleted = 0.0
  avg_particles = [0.0, 0.0, 0.0, 0.0]

  cum_sum_index = 0
  cum_sum =  particles[cum_sum_index][4]
  i = 0
  for  i in range(nr_of_particles):
    position = (random.random() + i) / nr_of_particles
    while position >= cum_sum:
      cum_sum_index+=1
      cum_sum += particles[cum_sum_index][4]
    
    new_particles.append(deepcopy(particles[cum_sum_index]))  
    avg_particles[0] += particles[cum_sum_index][0] * particles[cum_sum_index][4]
    avg_particles[1] += particles[cum_sum_index][1] * particles[cum_sum_index][4]
    cum_weight_seleted += particles[cum_sum_index][4]
    print(particles[cum_sum_index])

  avg_particles[0] /= cum_weight_seleted
  avg_particles[1] /= cum_weight_seleted

  std_x = 0.0
  std_y = 0.0
  for i in range(nr_of_particles):
      std_x += new_particles[i][4] * (new_particles[i][0] - avg_particles[0]) ** 2
      std_y += new_particles[i][4] * (new_particles[i][1] - avg_particles[1]) ** 2
  std_x /= cum_weight_seleted
  std_y /= cum_weight_seleted
  avg_particles[2] = math.sqrt(std_x)
  avg_particles[3] = math.sqrt(std_y)


  return new_particles, avg_particles


def predict(delta_t):
  global particles

  for particle in particles:
    oldparticle = deepcopy(particle)
    particle[0] += math.floor(particle[2] * delta_t * math.cos(particle[3]) + random.randint(0, 40) - 20) #should be gaussian but to computational
    particle[1] += math.floor(particle[2] * delta_t * math.sin(particle[3]) + random.randint(0, 40) - 20)
    particle[2] += math.floor((random.randint(0, 100) - 50) * delta_t)
    if (particle[2] < 0):
      particle[2] = 0
    elif particle[2] > max_speed:
      particle[2] = max_speed

    particle[3] += random.uniform(0, math.pi) - math.pi/2
    if (particle[3] < 0):
      particle[3] += math.pi * 2
    elif particle[3] > math.pi * 2:
      particle[3] -= math.pi * 2

    print(oldparticle, " -> " , particle)

def plot_particles(suffix):
  max_particle_w = 0.0
  for particle in particles:
    if (particle[4] > max_particle_w):
      max_particle_w = particle[4] 
  
  print("max_particle_w", max_particle_w)

  particle_patches = []
  for particle in particles:
    p = patches.Circle([particle[0], particle[1]], 5, color=cmap(particle[4] / max_particle_w))
    ax.add_patch(p)
    particle_patches.append(p)

  fig1.savefig('img/localization_pf_' + suffix + '.png', dpi=180, bbox_inches='tight')

  for p in particle_patches:
    p.remove()

##INIT
init_particles()
#set_particles()

for i in range(6):
  if ranges[i]  != -1 :    
    print("Range", i+1, ranges[i])

    #ranges
    circle_range = patches.Circle(real_node_positions[i+1],  ranges[i], facecolor='none', edgecolor=colors[i+1])
    ax.add_patch(circle_range)
    circle_ranges.append(circle_range)

#UPDATE
update_weights()
plot_particles("01")



print("RESAMPLE")
particles, position = resample(particles)

circle_estimation = patches.Circle(position, 100, facecolor='green', edgecolor='green')
circle_estimation.set_alpha(0.5)
ax.add_patch(circle_estimation)
plot_particles("02")
circle_estimation.remove()


print("PREDICT")
predict(1)

plot_particles("03")

#UPDATE
update_weights()
plot_particles("04")

print("RESAMPLE")
particles, position = resample(particles)

circle_estimation = patches.Circle(position, 100, facecolor='green', edgecolor='green')
circle_estimation.set_alpha(0.5)
ax.add_patch(circle_estimation)
plot_particles("05")
circle_estimation.remove()

print("PREDICT")
predict(1)

plot_particles("06")

print("POSITION", position)
