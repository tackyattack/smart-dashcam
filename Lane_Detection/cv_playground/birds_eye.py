import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
from math import *
import cv2

image = cv2.imread('road7.png')
image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
out_img=np.ndarray(shape=image.shape, dtype=image.dtype)

h = image.shape[0]
w = image.shape[1]

for y in range(0, h):
    for x in range(0, w):
        out_img[y,x] = [0,0,0]

def world_to_camera(H, x, y):
    origin = np.array([[x],
                      [y],
                      [1]])

    origin_transformed = np.dot(H, origin)
    origin_x = origin_transformed[0][0]/origin_transformed[2][0]
    origin_y = origin_transformed[1][0]/origin_transformed[2][0]
    return origin_x, origin_y

def calculate_transformation_matrix(w, h):
    t = -45*pi/180.0
    p = 0*pi/180.0

    # focal_pixel = (focal_mm / sensor_width_mm) * image_width_in_pixels
    fx = (35.0/22.3)*w
    fy = (35.0/14.9)*h

    # height in pixels
    # this should always be larger than image height I think or else
    # bottom floats off
    h_vert = 200

    R = np.array([[cos(p),   sin(p)*sin(t),     h_vert*sin(p)*sin(t)],
                  [-sin(p), cos(p)*sin(t), -h_vert*cos(p)*sin(t)],
                  [0, cos(t), h_vert*cos(t)]
                  ])
    C = np.array([[fx, 0, 0],
                  [0, fy, 0],
                  [0, 0, 1]
                  ])

    T = np.array([[1/1, 0, 0],
                  [0, 1/1, 0],
                  [0, 0, 1]
                  ])

    H = np.dot(C, R)
    H = np.dot(T, H)
    H_inv = np.linalg.inv(H)

    null, y_a = world_to_camera(H, 0, h/2)
    x_a, null = world_to_camera(H, -w, h/2)
    print(x_a)

    origin_x = x_a + w/2
    origin_y = y_a + h/2
    print(origin_x)
    print(origin_y)

    percent_height = 0.80
    null, top = world_to_camera(H, 0, h/2)
    null, bottom = world_to_camera(H, 0, -h/2*percent_height)
    scale_y = abs(top-bottom)/h
    print(scale_y)


    percent_width = 2
    left, null = world_to_camera(H, -w/2, h/2)
    right, null = world_to_camera(H, w/2, h/2)
    scale_x = abs(left-right)/w*percent_width
    print(scale_x)



    return H_inv, origin_x, origin_y, scale_x, scale_y


H_inv, origin_x, origin_y, scale_x, scale_y = calculate_transformation_matrix(w,h)

# loop over the image, pixel by pixel
for y in range(0, h):
    for x in range(0, w):

        pixel_pos = np.array([
                              [(x)*scale_x-w/2+origin_x],
                              [(y)*scale_y-h/2+origin_y],
                              [1]
                              ])


        world_pos = np.dot(H_inv, pixel_pos)

        world_x = int(world_pos[0][0]/world_pos[2][0] + w/2)
        world_z = int((world_pos[1][0]/world_pos[2][0]) + h/2)


        if (world_x > 0) and (world_x < w) and (world_z > 0) and (world_z < h):
            out_img[y, x] = image[world_z, world_x]
        else:
            out_img[y,x] = [0,0,0]


imgplot = plt.imshow(out_img)
plt.show()
