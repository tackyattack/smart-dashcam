import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
from math import *
import cv2
import os
import sys
script_path = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(script_path, '../src/pilanes'))
import BirdsEye

image = cv2.imread('combined.png')
image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
out_img=np.ndarray(shape=image.shape, dtype=image.dtype)

h = image.shape[0]
w = image.shape[1]

for y in range(0, h):
    for x in range(0, w):
        out_img[y,x] = [0,0,0]


bm = BirdsEye.BirdsEyeMath()
transform_matrix = bm.get_transformation_matrix(width=w, height=h, focal_length_mm=35.0,
                                        sensor_width_mm=22.3, sensor_height_mm=14.9, angle=45.0,
                                        camera_altitude_mm=2500)

print(transform_matrix)
for i in transform_matrix:
    for j in i:
        print(j)

# loop over the image, pixel by pixel
for y in range(0, h):
    for x in range(0, w):

        pixel_pos = np.array([
                              [(x)],
                              [(y)],
                              [1]
                              ])



        world_pos = np.dot(transform_matrix, pixel_pos)

        world_x = int(world_pos[0][0]/world_pos[2][0] + w/2)
        world_z = int((world_pos[1][0]/world_pos[2][0]) + h/2)


        if (world_x > 0) and (world_x < w) and (world_z > 0) and (world_z < h):
            out_img[y, x] = image[world_z, world_x]
        else:
            out_img[y,x] = [0,0,0]


imgplot = plt.imshow(out_img)
plt.show()
