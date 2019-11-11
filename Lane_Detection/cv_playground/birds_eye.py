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

# def world_to_camera(H, x, y):
#     origin = np.array([[x],
#                       [y],
#                       [1]])
#
#     origin_transformed = np.dot(H, origin)
#     origin_x = origin_transformed[0][0]/origin_transformed[2][0]
#     origin_y = origin_transformed[1][0]/origin_transformed[2][0]
#     return origin_x, origin_y
#
# def calculate_transformation_matrix(w, h):
#     t = -45*pi/180.0
#     p = 0*pi/180.0
#
#     physical_focal = 35.0
#     sensor_width_mm = 22.3
#     sensor_height_mm = 14.9
#
#     # focal_pixel = (focal_mm / sensor_width_mm) * image_width_in_pixels
#     fx = (physical_focal/sensor_width_mm)*w
#     fy = (physical_focal/sensor_height_mm)*h
#
#     # height in pixels
#     # this should always be larger than image height I think or else
#     # bottom floats off
#
#     # describes how camera coordinates transform to world
#     C = np.array([[fx, 0, 0],
#                   [0, fy, 0],
#                   [0, 0, 1]
#                   ])
#
#     physical_height_mm = 2800
#     # how many image planes does it take to get up to the camera's actual height
#     num_image_planes_y = physical_height_mm/sensor_height_mm
#     # multiply by the pixel size of the image plane to get the camera's
#     # vertical position in pixel units
#     cam_pixel_y = fy*num_image_planes_y
#     # create the matrix that goes from world to camera coordinates
#     world_to_cam_mat = np.linalg.inv(C)
#     world_space_cam_pos_3 = np.array([[0],
#                           [cam_pixel_y],
#                           [1]])
#     # transform world coordinates into camera space
#     cam_space_pos_3 = np.dot(world_to_cam_mat, world_space_cam_pos_3)
#     h_vert = cam_space_pos_3[1][0]/cam_space_pos_3[2][0]
#     print(h_vert)
#
#     R = np.array([[cos(p),   sin(p)*sin(t),     h_vert*sin(p)*sin(t)],
#                   [-sin(p), cos(p)*sin(t), -h_vert*cos(p)*sin(t)],
#                   [0, cos(t), h_vert*cos(t)]
#                   ])
#
#
#     H = np.dot(C, R)
#     H_inv = np.linalg.inv(H)
#
#     null, y_a = world_to_camera(H, 0, h/2)
#     x_a, null = world_to_camera(H, -w/2, h/2)
#     print(x_a)
#
#     origin_x = x_a
#     origin_y = y_a + h/2
#     print(origin_x)
#     print(origin_y)
#
#     percent_height = 1.00
#     null, top = world_to_camera(H, 0, h/2)
#     null, bottom = world_to_camera(H, 0, -h/2*percent_height)
#     scale_y = abs(top-bottom)/h
#     scale_y = 12
#     print(scale_y)
#
#
#     percent_width = 1
#     left, null = world_to_camera(H, -w/2, h/2)
#     right, null = world_to_camera(H, w/2, h/2)
#     scale_x = abs(left-right)/w*percent_width
#     print(scale_x)
#
#
#
#     return H_inv, origin_x, origin_y, scale_x, scale_y
#
#
# H_inv, origin_x, origin_y, scale_x, scale_y = calculate_transformation_matrix(w,h)
#
# X = np.array([[scale_x, 0, -w/2+origin_x],
#               [0, scale_y, -h/2+origin_y],
#               [0, 0, 1]
#               ])
#
# transform_matrix = np.dot(H_inv, X)

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
