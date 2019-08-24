import math


def intersectTwoCircles(x1,y1,r1, x2,y2,r2) :
  centerdx = x1 - x2
  centerdy = y1 - y2
  R = math.sqrt(centerdx * centerdx + centerdy * centerdy)
  print (r1, r2, R)

  if (not (abs(r1 - r2) <= R and R <= r1 + r2)) : #// no intersection
    return [] # empty list of results

  # intersection(s) should exist

  R2 = R*R
  R4 = R2*R2
  a = (r1*r1 - r2*r2) / (2 * R2)
  r2r2 = (r1*r1 - r2*r2)
  c = math.sqrt(2 * (r1*r1 + r2*r2) / R2 - (r2r2 * r2r2) / R4 - 1)

  print("a, c", a, c)

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


result = intersectTwoCircles(15, 480, 391, 15, 0, 273)

print (result); 