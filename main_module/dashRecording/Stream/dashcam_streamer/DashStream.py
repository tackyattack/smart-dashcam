#!/usr/bin/env python

# for help:
# ./DashcamRecorder.py -h

import argparse
from picamera import PiCamera
from picamera import PiRenderer
from picamera import encoders
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

server_init = stream_lib.server_init
server_init.argtypes = [ctypes.c_int, ctypes.c_uint32]
server_init.restype = None

server_loop = stream_lib.server_loop
server_loop.argtypes = None
server_loop.restype = None

def print_hex_block(block, len):
    a = list(block)
    for i in range(len):
        print hex(ord(a[i])),

    print("----")

camera = PiCamera()
camera.resolution = (1640, 922)
camera.framerate = 20

class StreamEncoder(encoders.PiVideoEncoder):
    def _callback(self, port, buf):
        record_bytes_stream(buf.data, len(buf.data))
        return bool(buf.flags & mmal.MMAL_BUFFER_HEADER_FLAG_EOS)

def create_stream_encoder(camera, splitter_port, format, resize, quality):
    output = 'dummy.h264'
    with camera._encoders_lock:
        camera_port, output_port = camera._get_ports(True, splitter_port)
        encoder_format = camera._get_video_format(output, format)
        encoder = StreamEncoder(parent=camera, camera_port=camera_port, input_port=output_port,
                                            format=encoder_format, resize=resize, quality=quality)
        camera._encoders[splitter_port] = encoder
    try:
        encoder.start(output)
    except Exception as e:
        encoder.close()
        with camera._encoders_lock:
            del encoder
        raise

create_stream_encoder(camera=camera, splitter_port=3, format='h264', resize=(240, 160), quality=30)

server_init(8080, 100)
try:
    while True:
        server_loop()

except KeyboardInterrupt:
    pass
run = False
camera.close()
