import numpy as np
from math import *
import cv2

# http://www.cim.mcgill.ca/~langer/558/4-cameramodel.pdf

# all these in meter
# focal length
# rpi is 3.04mm
f = 3.0/1000.0
# number of pixels per m
# t2i = 4.29
# rpi = 1.12
pixel_size_um = 1.12
m = 1.0/(pixel_size_um/1000000.0)
# use these to offset since
# 0,0 in the final output with cx=0, cy=0
# means center of screen, but we don't index from center in images
# You probably want to keep it in the center though since it's hard to
# tell exactly where the crop will be, but it's always centered at 0,0
cx = 0
cy = 0
intrinsic = np.array([[m*f, 0, cx],
                      [0, m*f, cy],
                      [0, 0, 1],
                     ])
print(intrinsic)
# camera angle
rx_angle = -20.0*pi/180.0
# camera location in meter
tx = 0.0
ty = 2.1
tz = 0.0
rotation = np.array([[1, 0, 0],
                      [0, cos(rx_angle), -1*sin(rx_angle)],
                      [0, sin(rx_angle), cos(rx_angle)],
                     ])
translation = np.array([[1, 0, 0,-tx],
                      [0, 1, 0, -ty],
                      [0, 0, 1, -tz],
                      ])

# real world position in meter
world_pos = np.array([[0],
                      [0],
                      [9.5],
                      [1]
                     ])
extrinsic = np.dot(rotation, translation)
world_to_camera = np.dot(intrinsic, extrinsic)
pixel_location = np.dot(world_to_camera, world_pos)
# divide by w to go back to homogeneous
sensor_y = pixel_location[1][0] / pixel_location[2][0]
# RPi pixel size in meter
print(pixel_location)
print(pixel_location[1][0])
print(sensor_y)
#print(sensor_y + 3456/2)

# transform angle calculation
close = 3.0
far = 20.0
height = 2.1
mid = (close+far)/2.0
angle_t = atan(mid/height)*180.0/pi
print(angle_t)
