import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
from math import *
import cv2

image = cv2.imread('road2.png')
image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
out_img=np.ndarray(shape=image.shape, dtype=image.dtype)

h = image.shape[0]
w = image.shape[1]

# loop over the image, pixel by pixel
for y in range(0, h):
    for x in range(0, w):
        t = 70*pi/180.0
        sx = 1
        sy = 8
        f = 300

        h = np.array([[sx*1, 0, 0, 0],
                      [0, sy*cos(t), -sin(t), 0],
                      [0, sin(t), cos(t), 0],
                      [0, 0, 0, 1]
                      ]).reshape(4, 4)


        # h = np.array([[1, 0, 0, 0],
        #               [0, 1, 0, 0],
        #               [0, 0, 1, 0],
        #               [0, 0, 0, 1]
        #               ]).reshape(4, 4)

        #X = x - (600/2)
        #Y = y - (400/2)
        X = x #+ 600/2
        Y = y #+ 400/2
        a = np.array([X, Y, 1000, 1]).reshape(4, 1)
        o = np.dot(h, a)

        # make a pinhole camera to create the 3D object
        p = o[0][0]  # x
        q = o[1][0]  # y
        r = o[2][0]  # z
        # u = f*p/(f+r)
        # v = f*q/(f+r)

        #https://www.scratchapixel.com/lessons/3d-basic-rendering/computing-pixel-coordinates-of-3d-point/mathematics-computing-2d-coordinates-of-3d-points?url=3d-basic-rendering/computing-pixel-coordinates-of-3d-point/mathematics-computing-2d-coordinates-of-3d-points
        # had to be negative or else you would get invalid values
        u = int(p*f/(1*r))#-int((600/2))
        v = int(q*f/(1*r))#-int((400/2))

        # only map values in the right range
        if (v > 0) and (v < 400) and (u > 0) and (u < 600) and (r > 0):
            out_img[v,u] = image[y,x]


imgplot = plt.imshow(out_img)
plt.show()

img=mpimg.imread('road1.png')
out_img=np.ndarray(shape=img.shape, dtype=img.dtype)