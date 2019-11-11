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

    def calculate_transformation_matrix(self, theta_angle, w, h, focal_length_mm,
                                        sensor_width_mm, sensor_height_mm, camera_altitude_mm):
        t = theta_angle*pi/180.0
        p = 0*pi/180.0

        physical_focal = focal_length_mm

        # focal_pixel = (focal_mm / sensor_width_mm) * image_width_in_pixels
        fx = (physical_focal/sensor_width_mm)*w
        fy = (physical_focal/sensor_height_mm)*h

        # describes how camera coordinates transform to world
        C = np.array([[fx, 0, 0],
                      [0, fy, 0],
                      [0, 0, 1]
                      ])

        physical_height_mm = camera_altitude_mm
        # how many image planes does it take to get up to the camera's actual height
        num_image_planes_y = physical_height_mm/sensor_height_mm
        # multiply by the pixel size of the image plane to get the camera's
        # vertical position in pixel units
        cam_pixel_y = fy*num_image_planes_y
        # create the matrix that goes from world to camera coordinates
        world_to_cam_mat = np.linalg.inv(C)
        world_space_cam_pos_3 = np.array([[0],
                              [cam_pixel_y],
                              [1]])
        # transform world coordinates into camera space
        null, h_vert = self.world_to_camera(world_to_cam_mat, 0, cam_pixel_y)
        print(h_vert)
        # height in pixels
        # this should always be larger than image height I think or else
        # bottom floats off


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

        # How much of the final output to compress down into the view window.
        # The more you compress it, the more sensitive it is to deviations from
        # the given homography angle/height. I.e, the lanes further out will bounce.
        # It's not necessary to have the entire transformation compressed down, but it should
        # be enough to include dotted lane + a partial of the next so that framerate aliasing
        # doesn't cause fast lanes to become invisible.
        percent_height = 0.25
        null, top = self.world_to_camera(H, 0, h/2)
        null, bottom = self.world_to_camera(H, 0, -h/2*percent_height)
        scale_y = abs(top-bottom)/h
        # for now, it looks like not scaling is best
        # because parameters don't have to be as precise
        #scale_y = 1
        print(scale_y)


        percent_width = 1
        left, null = self.world_to_camera(H, -w/2, h/2)
        right, null = self.world_to_camera(H, w/2, h/2)
        scale_x = abs(left-right)/w*percent_width
        print(scale_x)

        return H_inv, origin_x, origin_y, scale_x, scale_y

    def get_transformation_matrix(self, width, height, angle, focal_length_mm,
                                  sensor_width_mm, sensor_height_mm, camera_altitude_mm):
        H_inv, origin_x, origin_y, scale_x, scale_y = self.calculate_transformation_matrix(-1*angle, width, height,
                                                      focal_length_mm, sensor_width_mm, sensor_height_mm, camera_altitude_mm)

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
