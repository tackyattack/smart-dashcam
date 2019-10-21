#!/usr/bin/env python

# for help:
# ./DashcamRecorder.py -h

import argparse
from picamera import PiCamera
from picamera import PiRenderer
from picamera import mmal, mmalobj as mo
import threading
import os
from time import sleep
import subprocess
import Queue
import sys
import io
import socket
import ctypes

root_path = os.path.dirname(os.path.realpath(__file__))
stream_lib = ctypes.CDLL(os.path.join(root_path, 'stream.so'))
record_bytes_stream = stream_lib.record_bytes
record_bytes_stream.argtypes = [ctypes.c_char_p, ctypes.c_uint32]
record_bytes_stream.restype = None

get_next_chunk = stream_lib.get_next_chunk
get_next_chunk.argtypes = [ctypes.c_char_p]
get_next_chunk.restype = ctypes.c_int
queue_chunk = (ctypes.c_char*(4000))()


server_init = stream_lib.server_init
server_init.argtypes = [ctypes.c_int]
server_init.restype = None

server_loop = stream_lib.server_loop
server_loop.argtypes = None
server_loop.restype = None





def print_hex_block(block, len):
    a = list(block)
    for i in range(len):
        print hex(ord(a[i])),

    print("----")


f = open('splitter.h264', 'w+')
f.close()
f = open('splitter.h264', 'ab')



def video_callback(port, buf):
    # if (SPS is None) or (PPS is None):
    #     log_data(buf.data)
    # else:
    #     write_block(buf.data)
    #
    # print(q.qsize())
    #print('mew')
    #record_bytes(buf.data)
    record_bytes_stream(buf.data,len(buf.data))
    return False

dummy_streamA = io.BytesIO()
dummy_streamB = io.BytesIO()



camera = PiCamera()
camera.resolution = (1640, 922)
camera.framerate = 20
camera.start_recording('dummy.h264', format='h264')

camera.start_recording('test.h264', format='h264', splitter_port=3,
                        resize=(240, 160), quality=20)

camera._encoders[3].encoder.outputs[0].disable()
camera._encoders[3].encoder.outputs[0].enable(video_callback)
# camera.stop_recording()
# sleep(1)
# camera.stop_recording()

server_init(8080)
try:
    while True:
        server_loop()

except KeyboardInterrupt:
    pass
run = False
camera.stop_recording()
camera.close()
f.close()
