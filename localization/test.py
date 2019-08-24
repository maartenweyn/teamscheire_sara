counter = [0, 2, 100, 1, 100, 100]
counter = [0, 2, 1, 1, 1, 1]

use_measurement_threshold = 6 * 4

all = []
  
for i in range(5):
  if (counter[i] < use_measurement_threshold):
    for j in range(i+1, 6):
      if (counter[j] < use_measurement_threshold):
        print (i, j)
        all.append([i, j])

print (all)

print (len(all))