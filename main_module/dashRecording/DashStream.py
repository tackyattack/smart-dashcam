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

q = Queue.Queue(maxsize=500)

f = open('splitter.h264', 'w+')
f.close()
f = open('splitter.h264', 'ab')

SPS = None
PPS = None

def get_next_NAL_unit(start_index, data):
    i = start_index
    header = None
    while i < len(data)-4:
        d0 = ord(data[i+0])
        d1 = ord(data[i+1])
        d2 = ord(data[i+2])
        d3 = ord(data[i+3])
        d4 = ord(data[i+4])
        if (d0 == 0x00) and (d1 == 0x00) and (d2 == 0x00) and (d3 == 0x01):
            header = d4
            break
        i = i + 1
    return i, header

def get_NAL_unit(start_index, data):
    i=start_index
    header = None
    NAL_begin, header1 = get_next_NAL_unit(i, data)
    NAL_end, header2 = get_next_NAL_unit(NAL_begin+1, data)
    # NAL is last unit
    if header2 is None:
        NAL_end = (len(data)-1) - NAL_begin
    NAL_unit = data[NAL_begin:NAL_end]
    return NAL_end-1, header1, NAL_unit


def get_NAL_units(data):
    i=0
    exit = False
    while not exit:
        i, header, unit = get_NAL_unit(i, data)

        if(header == 0x28):
            print('bing')
            print(hex(ord(unit[-1:])))
        if(header is None):
            exit=True

magic_pattern = [0x00, 0x00, 0x00, 0x01]
pattern_pointer = 0
current_NAL = ''
NAL_list = []
def record_byte(data_char):
    global pattern_pointer
    global current_NAL
    global NAL_list
    global SPS
    global PPS
    new_NAL = False
    current_NAL += data_char
    if magic_pattern[pattern_pointer] == ord(data_char):
        pattern_pointer = pattern_pointer + 1
    else:
        pattern_pointer = 0
    if pattern_pointer == len(magic_pattern):
        pattern_pointer = 0
        new_NAL = True

    if new_NAL:
        if len(current_NAL) > 4:
            NAL = chr(0x00) + chr(0x00) + chr(0x00) + chr(0x01) + current_NAL[:-5]
            #NAL_list.append(hex(ord(NAL[3])))
            if ord(current_NAL[0]) == 0x28:
                PPS = NAL
            if ord(current_NAL[0]) == 0x27:
                SPS = NAL
        current_NAL = ''
        new_NAL = False


def log_data(buffer):
    for c in buffer:
        record_byte(c)

first_packet = True
def write_block(buffer):
    global SPS
    global PPS
    global first_packet
    if first_packet:
        block = SPS + PPS + buffer
        first_packet = False
    else:
        block = buffer
    for c in block:
        x =  q.put(c)
        if x:
            print('overrun')
    f.write(block)

def video_callback(port, buf):
    if (SPS is None) or (PPS is None):
        log_data(buf.data)
    else:
        write_block(buf.data)

    print(q.qsize())
    return False

dummy_streamA = io.BytesIO()
dummy_streamB = io.BytesIO()



camera = PiCamera()
camera.resolution = (1640, 922)
camera.framerate = 20
camera.start_recording(dummy_streamA, format='h264')

camera.start_recording('test.h264', format='h264', splitter_port=3,
                        resize=(320, 240), quality=40)

camera._encoders[3].encoder.outputs[0].disable()
camera._encoders[3].encoder.outputs[0].enable(video_callback)
#camera.stop_recording()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('131.151.175.144', 8080)
sock.bind(server_address)
sock.listen(1)
while True:
    # Wait for a connection
    print >>sys.stderr, 'waiting for a connection'
    connection, client_address = sock.accept()
    try:
        print >>sys.stderr, 'connection from', client_address
        data = connection.recv(1)
        # Receive the data in small chunks and retransmit it
        while True:
            val = q.get()
            if val:
                connection.sendall(val)

    finally:
        # Clean up the connection
        connection.close()

sleep(10)
camera.stop_recording()
camera.close()
f.close()
print(NAL_list)
