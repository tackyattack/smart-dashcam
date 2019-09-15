import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
from math import *
import cv2

image = cv2.imread('road3.png')
image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
out_img=np.ndarray(shape=image.shape, dtype=image.dtype)

h = image.shape[0]
w = image.shape[1]

# loop over the image, pixel by pixel
for y in range(0, h):
    for x in range(0, w):
        t = 45*pi/180.0
        sx = 18
        sy = 2
        f = 0.1


        # a = np.array([[1, 0, 0, 0],
        #               [0, 1, 0, 0],
        #               [0, 0, 1, 0],
        #               [0, 0, 0, 1]
        #               ])

        camera_matrix = np.array([[1, 0, 0, 0],
                                  [0, 1, 0, -100],
                                  [0, 0, 1, -200],
                                  [0, 0, 0, 1]
                                 ])
        camera_location = np.array([0, 0, 1, 1])


        a = np.array([[sy, 0, 0, 0],
                      [0, sx*cos(t), sin(t), 0],
                      [0, sin(t), cos(t), 0],
                      [0, 0, 0, 1]
                      ])


        pixel_location = np.array([x-600/2, y-400/2, 0, 1])

        o = np.dot(a, pixel_location)

        cam_mat_inv = np.linalg.inv(camera_matrix)
        o = np.dot(cam_mat_inv, o)


        # make a pinhole camera to create the 3D object
        p = o[0] # x
        q = o[1]  # y
        r = o[2]  # z
        # u = f*p/(f+r)
        # v = f*q/(f+r)

        canvas_width = 2
        canvas_height = 2
        #https://www.scratchapixel.com/lessons/3d-basic-rendering/computing-pixel-coordinates-of-3d-point/mathematics-computing-2d-coordinates-of-3d-points?url=3d-basic-rendering/computing-pixel-coordinates-of-3d-point/mathematics-computing-2d-coordinates-of-3d-points
        # had to be negative or else you would get invalid values
        screen_x = p*f/(-1.0*r)#-int((600/2))
        screen_y = q*f/(-1.0*r)#-int((400/2))

        if (abs(screen_x) > canvas_width) or (abs(screen_y) > canvas_height):
            pass
        else:

            normal_x = (screen_x*1.0 + canvas_width/2.0)/canvas_width
            normal_y = (screen_y*1.0 + canvas_height/2.0)/canvas_height
            u = floor(normal_x * 600.0)
            v = floor((1 - normal_y) * 400.0)


            # only map values in the right range
            if (v > 0) and (v < 400) and (u > 0) and (u < 600):
                out_img[v,u] = image[y,x]
                # filtering since the top and bottom pixel will get rounded
                # so it might land on the wrong one, so just duplicate it
                out_img[v+1,u] = image[y,x]
                out_img[v+2, u] = image[y, x]
                out_img[v-2, u] = image[y, x]
                out_img[v-1, u] = image[y, x]


imgplot = plt.imshow(out_img)
plt.show()

img=mpimg.imread('road1.png')
out_img=np.ndarray(shape=img.shape, dtype=img.dtype)