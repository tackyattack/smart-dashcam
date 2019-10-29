import numpy as np
import ctypes
from math import *

class BirdsEyeMath:

    def __init__(self):
        pass

    def world_to_camera(self, H, x, y):
        origin = np.array([[x],
                          [y],
                          [1]])

        origin_transformed = np.dot(H, origin)
        origin_x = origin_transformed[0][0]/origin_transformed[2][0]
        origin_y = origin_transformed[1][0]/origin_transformed[2][0]
        return origin_x, origin_y

    def calculate_transformation_matrix(self, theta_angle, w, h, camera_pixel_altitude):
        t = theta_angle*pi/180.0
        p = 0*pi/180.0

        # focal_pixel = (focal_mm / sensor_width_mm) * image_width_in_pixels
        fx = (35.0/22.3)*w
        fy = (35.0/14.9)*h

        # height in pixels
        # this should always be larger than image height I think or else
        # bottom floats off
        h_vert = camera_pixel_altitude

        # https://pdfs.semanticscholar.org/4964/9006f2d643c0fb613db4167f9e49462546dc.pdf
        # transform -> rotate -> projection
        R = np.array([[cos(p),   sin(p)*sin(t), sin(p)*sin(t)],
                      [-sin(p),  cos(p)*sin(t), -cos(p)*sin(t)],
                      [0,        cos(t),        cos(t)]
                      ])
        T = np.array([[1, 0, 0],
                      [0, 1, 0],
                      [0, 0, h_vert]
                      ])
        C = np.array([[fx, 0, 0],
                      [0, fy, 0],
                      [0, 0, 1]
                      ])
        # H = CRT
        H = np.dot(R, T)
        H = np.dot(C, H)
        H_inv = np.linalg.inv(H)

        null, y_a = self.world_to_camera(H, 0, h/2)
        x_a, null = self.world_to_camera(H, -w/2, h/2)
        print(x_a)

        origin_x = x_a
        origin_y = y_a + h/2
        print(origin_x)
        print(origin_y)

        percent_height = 0.10
        null, top = self.world_to_camera(H, 0, h/2)
        null, bottom = self.world_to_camera(H, 0, -h/2*percent_height)
        scale_y = abs(top-bottom)/h
        # for now, it looks like not scaling is best
        # because parameters don't have to be as precise
        scale_y = 1
        print(scale_y)


        percent_width = 1
        left, null = self.world_to_camera(H, -w/2, h/2)
        right, null = self.world_to_camera(H, w/2, h/2)
        scale_x = abs(left-right)/w*percent_width
        print(scale_x)

        return H_inv, origin_x, origin_y, scale_x, scale_y

    def get_transformation_matrix(self, width, height, angle, camera_pixel_altitude):
        H_inv, origin_x, origin_y, scale_x, scale_y = self.calculate_transformation_matrix(-1*angle, width, height, camera_pixel_altitude)

        X = np.array([[scale_x, 0, -width/2+origin_x],
                      [0, scale_y, -height/2+origin_y],
                      [0, 0, 1]
                      ])

        transform_matrix = np.dot(H_inv, X)
        return transform_matrix

    def numpy_to_float_mat(self, numpy_array, sz):
        numpy_array = np.transpose(numpy_array)
        mat = (ctypes.c_float*(sz))()
        mat_cnt = 0
        for i in numpy_array:
            for j in i:
                mat[mat_cnt] = j
                mat_cnt = mat_cnt + 1
        return mat
