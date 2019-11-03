
# Maarten Weyn
# maarten@wesdec.be

import matplotlib.pyplot as plt
import matplotlib.patches as patches
import math
import numpy as np
import itertools
import random
from copy import deepcopy


field = [2060, 3540]
field_size_marging  = 200


real_node_positions = {
  1: [0, 0],
  2: [2060, 0],
  3: [2110, 1730], 
  4: [2060, 3540], 
  5: [0, 3540], 
  6: [-50, 1740]}

letters = {
  "A":  [field[0] / 2, 0],
  "K" : [0, field[1] * 0.15], 
  "E" : [0, field[1] / 2], 
  "H" : [0, field[1] * 0.85], 
  "C" : [field[0] / 2, field[1]], 
  "M" : [field[0] , field[1] * 0.85], 
  "B" : [field[0] , field[1] / 2], 
  "F" : [field[0] , field[1] * 0.15], 
  "D" : [field[0] / 2, field[1] * 0.15], 
  "X" : [field[0] / 2, field[1] / 2], 
  "G" : [field[0] / 2, field[1] * 0.85]}

print ("A", letters["A"])
print ("K", letters["K"])
print ("E", letters["E"])
print ("H", letters["H"])
print ("C", letters["C"])
print ("M", letters["M"])
print ("B", letters["B"])
print ("F", letters["F"])
print ("D", letters["D"])
print ("X", letters["X"])
print ("G", letters["G"])

fig1 = plt.figure()
ax = fig1.add_subplot(111, aspect='equal')

ax.set_xlim(-200, field[0] + 200)
ax.set_ylim(-200, field[1] + 200)



rect = patches.Rectangle([0, 0], field[0], field[1], fill=False)
ax.add_patch(rect)

for key in letters:
  ax.text(letters[key][0], letters[key][1], key, fontsize=10, horizontalalignment='center', verticalalignment='center', color='r')

for key in real_node_positions:
  ax.text(real_node_positions[key][0], real_node_positions[key][1], key, fontsize=12, horizontalalignment='center', verticalalignment='center', color='b')
  circle = patches.Circle(real_node_positions[key], 0.5, facecolor='y')
  ax.add_patch(circle)
    


fig1.savefig('img/map.png', dpi=180, bbox_inches='tight')

