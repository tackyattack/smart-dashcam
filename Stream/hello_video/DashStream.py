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


def print_hex_block(block, len):
    a = list(block)
    for i in range(len):
        print hex(ord(a[i])),

    print("----")


q = Queue.Queue(maxsize=100)

f = open('splitter.h264', 'w+')
f.close()
f = open('splitter.h264', 'ab')

SPS = None
PPS = None

magic_pattern = [0x00, 0x00, 0x00, 0x01]
pattern_pointer = 0
current_NAL = ''
NAL_list = []
NAL_queue = Queue.Queue(maxsize=50)
signal = 0
def record_bytes(data_buffer):
    global pattern_pointer
    global current_NAL
    global NAL_list
    global SPS
    global PPS
    global signal
    pattern_len = len(magic_pattern)
    for data_char in data_buffer:
        new_NAL = False
        current_NAL += data_char
        if magic_pattern[pattern_pointer] == ord(data_char):
            pattern_pointer = pattern_pointer + 1
        else:
            pattern_pointer = 0
        if pattern_pointer == pattern_len:
            pattern_pointer = 0
            signal = signal+1

        if signal == 2:
            signal = 1
            new_NAL = True

        if new_NAL:
            NAL = current_NAL[:-4]
            #print(len(NAL))
            #print_hex_block(NAL, len(NAL))
            #NAL_list.append(hex(ord(NAL[3])))
            if (PPS is not None) and (SPS is not None):
                try:
                    NAL_queue.put(NAL, block=False)
                    #print(len(NAL))
                except Queue.Full:
                    print('NAL overrun')
                    #null = NAL_queue.get()
                    #NAL_queue.queue.clear()
                    #NAL_queue.put(NAL, block=False)
                    #pass

            if ord(NAL[4]) == 0x28:
                if PPS is None:
                    NAL_queue.put(NAL)
                PPS = NAL
            if ord(NAL[4]) == 0x27:
                if SPS is None:
                    NAL_queue.put(NAL)
                SPS = NAL

            current_NAL = chr(0x00)+chr(0x00)+chr(0x00)+chr(0x01)
            new_NAL = False


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
        x =  q.put(c, block=True)
        if x:
            print('overrun')
    f.write(block)

def video_callback(port, buf):
    # if (SPS is None) or (PPS is None):
    #     log_data(buf.data)
    # else:
    #     write_block(buf.data)
    #
    # print(q.qsize())
    print('mew')
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
                        resize=(320, 240), quality=10)

camera._encoders[3].encoder.outputs[0].disable()
camera._encoders[3].encoder.outputs[0].enable(video_callback)
# camera.stop_recording()
# sleep(1)
# camera.stop_recording()


run = True
def start():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('131.151.175.144', 8080)
    sock.bind(server_address)
    sock.listen(1)
    while run:
        # Wait for a connection
        print >>sys.stderr, 'waiting for a connection'
        connection, client_address = sock.accept()
        try:
            print >>sys.stderr, 'connection from', client_address
            #data = connection.recv(1)
            # Receive the data in small chunks and retransmit it
            while run:
                cnt = 0
                val = ''
                #val = NAL_queue.get(block=False)
                # while len(val) < 100:
                #     val += NAL_queue.get(block=True)

                while len(val) < 100:
                    # try:
                    #     data = NAL_queue.get(block=False)
                    # except Queue.Empty:
                    #     data = None
                    # if data is not None:
                    #     val+=data
                    if get_next_chunk(queue_chunk) == 1:
                        for i in range(100):
                            val+=queue_chunk[i]

                #print("about to send " + str(len(val)))
                #print_hex_block(val, len(val))
                total_sent = 0
                msg_len = len(val)
                while total_sent < msg_len:
                    sent = connection.send(val[total_sent:])
                    if sent == 0:
                        raise RuntimeError('broken socket')
                    total_sent = total_sent + sent
                print('sent ' + str(total_sent))

        finally:
            # Clean up the connection
            connection.close()


try:
    start()
    #val = ''
    #val = NAL_queue.get(block=False)
    #while len(val) < 100:
        #val += q.get(block=True)
    #val = ''
except KeyboardInterrupt:
    pass
run = False
camera.stop_recording()
camera.close()
f.close()
