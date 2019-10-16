import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
from math import *
import cv2

image = cv2.imread('road3.png')
image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
out_img=np.ndarray(shape=image.shape, dtype=image.dtype)

h = image.shape[0]
w = image.shape[1]

for y in range(0, h):
    for x in range(0, w):
        out_img[y,x] = [0,0,0]

# loop over the image, pixel by pixel
for y in range(0, h):
    for x in range(0, w):
        t = 45*pi/180.0
        p = 0*pi/180.0

        # focal_pixel = (focal_mm / sensor_width_mm) * image_width_in_pixels
        fx = (35.0/22.3)*600
        fy = (35.0/14.9)*400

        h = 250

        # world_pos = np.array([[x-600/2],
        #                       [y-400/2],
        #                       [1]])

        R = np.array([[cos(p),   sin(p)*sin(t),     h*sin(p)*sin(t)],
                      [-sin(p), cos(p)*sin(t), -h*cos(p)*sin(t)],
                      [0, cos(t), h*cos(t)]
                      ])
        C = np.array([[fx, 0, 0],
                      [0, fy, 0],
                      [0, 0, 1]
                      ])

        H = np.dot(C, R)
        H_inv = np.linalg.inv(H)

        # figure out where the origin is

        origin = np.array([[-300],
                          [-200],
                          [1]])

        origin_transformed = np.dot(H, origin)
        origin_x = origin_transformed[0][0]/origin_transformed[2][0]
        origin_y = origin_transformed[1][0]/origin_transformed[2][0]
        #print(origin_y)
        scale_x = abs(origin_x/300)
        scale_y = abs(origin_x/200)

        pixel_pos = np.array([
                              [(x)*scale_x-600/2+origin_x],
                              [(y)*scale_y-400/2+origin_y],
                              [1]
                              ])

        world_pos = np.dot(H_inv, pixel_pos)
        world_x = int(world_pos[0][0]/world_pos[2][0] + 600/2)
        world_z = int((world_pos[1][0]/world_pos[2][0] + 400/2))

        #print(world_x)
        #print(world_z)

        if (world_x > 0) and (world_x < 600) and (world_z > 0) and (world_z < 400):
            out_img[y, x] = image[world_z, world_x]
        else:
            out_img[y,x] = [0,0,0]
        #
        # t = 45*pi/180.0
        # sx = 18
        # sy = 2
        # f = 0.1


        # # a = np.array([[1, 0, 0, 0],
        # #               [0, 1, 0, 0],
        # #               [0, 0, 1, 0],
        # #               [0, 0, 0, 1]
        # #               ])
        #
        # camera_matrix = np.array([[1, 0, 0, 0],
        #                           [0, 1, 0, -100],
        #                           [0, 0, 1, -200],
        #                           [0, 0, 0, 1]
        #                          ])
        # camera_location = np.array([0, 0, 1, 1])
        #
        #
        # a = np.array([[sy, 0, 0, 0],
        #               [0, sx*cos(t), sin(t), 0],
        #               [0, sin(t), cos(t), 0],
        #               [0, 0, 0, 1]
        #               ])
        #
        #
        # pixel_location = np.array([x-600/2, y-400/2, 0, 1])
        #
        # o = np.dot(a, pixel_location)
        #
        # cam_mat_inv = np.linalg.inv(camera_matrix)
        # o = np.dot(cam_mat_inv, o)
        #
        #
        # # make a pinhole camera to create the 3D object
        # p = o[0] # x
        # q = o[1]  # y
        # r = o[2]  # z
        # # u = f*p/(f+r)
        # # v = f*q/(f+r)
        #
        # canvas_width = 2
        # canvas_height = 2
        # #https://www.scratchapixel.com/lessons/3d-basic-rendering/computing-pixel-coordinates-of-3d-point/mathematics-computing-2d-coordinates-of-3d-points?url=3d-basic-rendering/computing-pixel-coordinates-of-3d-point/mathematics-computing-2d-coordinates-of-3d-points
        # # had to be negative or else you would get invalid values
        # screen_x = p*f/(-1.0*r)#-int((600/2))
        # screen_y = q*f/(-1.0*r)#-int((400/2))
        #
        # if (abs(screen_x) > canvas_width) or (abs(screen_y) > canvas_height):
        #     pass
        # else:
        #
        #     normal_x = (screen_x*1.0 + canvas_width/2.0)/canvas_width
        #     normal_y = (screen_y*1.0 + canvas_height/2.0)/canvas_height
        #     u = floor(normal_x * 600.0)
        #     v = floor((1 - normal_y) * 400.0)
        #
        #     # w=600
        #     # h=400
        #     # zshift=-200
        #     # vshift=-100
        #     # u = -w*((f*sy*(w/2 - x))/(2*(zshift + sin(t)*(h/2 - y))) - 1/2)
        #     # v = h*((f*(vshift + sx*cos(t)*(h/2 - y)))/(2*(zshift + sin(t)*(h/2 - y))) + 1/2)
        #     # u=int(u)
        #     # v=int(v)

        # w=600
        # h=400
        # zshift=-200
        # vshift=-1
        # u=x
        # v=y
        # input_x = (2*h*vshift*w*sin(t) - 4*h*u*vshift*sin(t) + h*sy*w*w*sin(t) - 2*v*sy*w*w*sin(t) - 2*h*sx*w*zshift*cos(t) + 4*h*u*sx*zshift*cos(t) + f*h*sx*sy*w*w*cos(t))/(2*sy*w*(h*sin(t) - 2*v*sin(t) + f*h*sx*cos(t)))
        # input_y = (h*h*sin(t) + 2*h*zshift - 4*v*zshift - 2*h*v*sin(t) + 2*f*h*vshift + f*h*h*sx*cos(t))/(2*h*sin(t) - 4*v*sin(t) + 2*f*h*sx*cos(t))
        # input_x = int(input_x)
        # input_y = int(input_y)
        # if (input_y > 0) and (input_y < 400) and (input_x > 0) and (input_x < 600):
        #     out_img[y,x] = image[input_y, input_x]
        # else:
        #     out_img[y,x] = [0,0,0]

            # # only map values in the right range
            # if (v > 0) and (v < 400) and (u > 0) and (u < 600):
            #     out_img[v,u] = image[y,x]
            #     # filtering since the top and bottom pixel will get rounded
            #     # so it might land on the wrong one, so just duplicate it
            #     out_img[(v+1)%400,u] = image[y,x]
            #     out_img[(v+2)%400, u] = image[y, x]
            #     out_img[(v-2)%400, u] = image[y, x]
            #     out_img[(v-1)%400, u] = image[y, x]


# u = -w*((f*sy*(w/2 - x))/(2*(zshift + sin(t)*(h/2 - y))) - 1/2)
# v = h*((f*(vshift + sx*cos(t)*(h/2 - y)))/(2*(zshift + sin(t)*(h/2 - y))) + 1/2)
# matlab solve:
# x = (2*h*vshift*w*sin(t) - 4*h*u*vshift*sin(t) + h*sy*w^2*sin(t) - 2*v*sy*w^2*sin(t) - 2*h*sx*w*zshift*cos(t) + 4*h*u*sx*zshift*cos(t) + f*h*sx*sy*w^2*cos(t))/(2*sy*w*(h*sin(t) - 2*v*sin(t) + f*h*sx*cos(t)))
# y = (h^2*sin(t) + 2*h*zshift - 4*v*zshift - 2*h*v*sin(t) + 2*f*h*vshift + f*h^2*sx*cos(t))/(2*h*sin(t) - 4*v*sin(t) + 2*f*h*sx*cos(t))
# where u,v is the output texture coords you're rendering

imgplot = plt.imshow(out_img)
plt.show()
