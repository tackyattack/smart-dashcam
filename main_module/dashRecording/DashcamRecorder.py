#!/usr/bin/env python

# for help:
# ./DashcamRecorder.py -h

import argparse
from picamera import PiCamera
from picamera import mmal, mmalobj as mo
from picamera import encoders
import threading
import os
import signal
from time import sleep
import subprocess
import Queue
import sys
import ctypes


# --- DEPRECATION: stdio stream since it's slow
# if you want to still use a pipe, run this with the --stream flag then use
# netcat on localhost to pipe it into cvlc

# note: the frame rate flag for some reason has to be double the framerate
# ./DashcamRecorder.py --stream | cvlc -vvv stream:///dev/stdin --sout '#standard{access=http,mux=ts,dst=:8090}' :demux=h264 :h264-fps=40
# or you can use rtsp
# ./DashcamRecorder.py --stream | cvlc -vvv stream:///dev/stdin --sout '#rtp{sdp=rtsp://:8554/}' :demux=h264 :h264-fps=40
# You can transcode too, but this is only needed if the network is too slow
# ./DashcamRecorder.py --stream | cvlc -vvv stream:///dev/stdin --sout '#transcode{vcodec=h264,vb=25,fps=20}:rtp{sdp=rtsp://:8554/}' :demux=h264 :h264-fps=40

# to play back
# omxplayer -o hdmi  rtsp://131.151.175.144:8554/

root_path = os.path.dirname(os.path.realpath(__file__))
stream_lib = ctypes.CDLL(os.path.join(root_path, 'Stream/dashcam_streamer/stream.so'))

record_bytes_stream = stream_lib.record_bytes
record_bytes_stream.argtypes = [ctypes.c_char_p, ctypes.c_uint32]
record_bytes_stream.restype = None

streamer_init = stream_lib.streamer_init
streamer_init.argtypes = [ctypes.c_int, ctypes.c_uint32]
streamer_init.restype = None


close_server = stream_lib.close_server
close_server.argtypes = None
close_server.restype = None

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


def getKey(item):
    return item[0]

def get_number_file(file):
    # dashcam_video_0.ext
    num_str = file.split('.')[-2].split('_')[-1]
    num = int(num_str)
    return num

def get_sorted_video_paths(path):
    file_paths = []
    for file in os.listdir(path):
        if file.endswith('.mp4') or file.endswith('.h264'):
            extracted_num = get_number_file(file)
            # store as tuples
            file_paths.append((extracted_num, os.path.join(path, file)))

    if len(file_paths) == 0:
        empty_paths = []
        return empty_paths

    file_paths = sorted(file_paths, key=getKey)
    paths_only = []
    for path in file_paths:
        paths_only.append(path[1])
    return paths_only

class Recorder:
    def __init__(self, record_path, record_width, record_height, recording_interval_s, max_size_mb, stream, port=8080, stream_width=240,
                 stream_height=160, stream_quality=30, stream_buffer=10000):
        self.camera = PiCamera()
        self.camera.resolution = (record_width, record_height)
        self.framerate = 20
        self.camera.framerate = self.framerate
        self.record_path = record_path
        self.terminate_event = threading.Event()
        self.record_interval_s = recording_interval_s
        self.recording_thread = None
        self.wrapping_thread = None
        self.current_recording_name = None
        self.wrapping_queue = Queue.Queue()
        self.max_size_mb = max_size_mb
        self.stream_width = stream_width
        self.stream_height = stream_height
        self.stream_quality = stream_quality
        self.stream_port = port
        self.stream_buffer = stream_buffer
        self.silent = False
        self.do_stream = stream
        if not os.path.exists(self.record_path):
            os.makedirs(self.record_path)

        if self.do_stream:
            streamer_init(self.stream_port, self.stream_buffer)
            create_stream_encoder(camera=self.camera, splitter_port=3, format='h264',
                                 resize=(self.stream_width, self.stream_height), quality=self.stream_quality)


    def get_camera(self):
        return self.camera

    def start_recorder(self):
        self.terminate_event.clear()
        self.recording_thread = threading.Thread(target=self.recording_thread_func)
        self.recording_thread.start()
        self.wrapping_thread = threading.Thread(target=self.wrapping_thread_func)
        self.wrapping_thread.start()


    def terminate(self):
        self.terminate_event.set()
        while((self.wrapping_thread is not None) or (self.recording_thread is not None)):
            sleep(0.25)
        if(self.do_stream):
            close_server()

    def cleanup(self):
        for file in os.listdir(self.record_path):
            if file.endswith('.h264'):
                os.remove(os.path.join(self.record_path, file))

    # size in bytes
    def get_size(self, start_path = '.'):
        total_size = 0
        for dirpath, dirnames, filenames in os.walk(start_path):
            for f in filenames:
                fp = os.path.join(dirpath, f)
                # skip if it is symbolic link
                if not os.path.islink(fp):
                    total_size += os.path.getsize(fp)
        return total_size

    def get_video_num(self):
        file_names = get_sorted_video_paths(self.record_path)
        if len(file_names) == 0:
            return 0

        most_recent_file = os.path.basename(file_names[-1])
        extracted_num = get_number_file(most_recent_file)

        return extracted_num + 1

    def recording_thread_func(self):
        is_recording = False
        while not self.terminate_event.is_set():
            record_name_path = os.path.join(self.record_path,
                                       'dashcam_video_{0}.h264'.format(self.get_video_num()))
            self.current_recording_name = record_name_path
            is_recording = True
            self.camera.start_recording(record_name_path)
            self.terminate_event.wait(self.record_interval_s)
            self.camera.stop_recording()
            is_recording = False
            if(not self.silent):
                print("DONE RECORDING " + record_name_path)
            self.wrapping_queue.put(record_name_path)

        if is_recording:
            self.camera.stop_recording()
        self.camera.close()
        if(not self.silent):
            print("recording thread closed")
        self.recording_thread = None

    def check_reduce(self):
        size_mb = self.get_size(self.record_path)/1000000
        if(size_mb > self.max_size_mb):
            file_paths = []
            file_paths = get_sorted_video_paths(self.record_path)
            if len(file_paths) == 0:
                return
            # delete oldest
            file_to_delete = file_paths[0]
            if(not self.silent):
                print("removing: " + file_to_delete)
            os.remove(file_to_delete)

    def wrapping_thread_func(self):
        while not self.terminate_event.is_set():
            while not self.wrapping_queue.empty() and not self.terminate_event.is_set():
                # check if file is ready
                file_to_wrap_path = self.wrapping_queue.get()
                if(not self.silent):
                    print("WRAPPING " + file_to_wrap_path)
                # 'xxx.h264'
                file_out_path = file_to_wrap_path[:-4] + 'mp4'
                cmd_str = 'ffmpeg -framerate {0} -i {1} -c:v copy -f mp4 {2}'.format(self.framerate,
                                                                                     file_to_wrap_path,
                                                                                     file_out_path)
                cmd = cmd_str.split()
                subprocess.call(cmd, stdout=open(os.devnull, 'wb'), stderr=open(os.devnull, 'wb'))
                os.remove(file_to_wrap_path)
                self.check_reduce()


            self.check_reduce()
            self.terminate_event.wait(0.1) # give the CPU some time to do other stuff

        if(not self.silent):
            print("wrapping thread closed")
        self.cleanup()
        self.wrapping_thread = None


def start_recording_command_line():
    parser = argparse.ArgumentParser(description='recording and streaming module for camera')
    parser.add_argument("-t", default=10, type=int, help='sets the recording interval between clips')
    parser.add_argument("-m", default=100, type=int, help='sets the max MB size of the record folder')
    parser.add_argument("-o", default='', help='sets the location for recordings folder')
    parser.add_argument("-rw", default=640, type=int, help='sets the record width')
    parser.add_argument("-rh", default=480, type=int, help='sets the record height')
    parser.add_argument("-sw", default=240, type=int, help='sets the stream width')
    parser.add_argument("-sh", default=160, type=int, help='sets the stream height')
    parser.add_argument("-sq", default=30, type=int, help='sets the stream quality (1:highest 40:lowest)')
    parser.add_argument("-p", default=8080, type=int, help='sets the stream port')
    parser.add_argument("-sb", default=10000, type=int, help='sets the stream buffer in bytes')
    parser.add_argument("--stream", action='store_true', help='stream h264 through stdout')
    args = parser.parse_args()

    record_path = os.path.join(args.o, 'recordings')
    record_inst = Recorder(record_path=record_path, record_width=args.rw, record_height=args.rh,
                           recording_interval_s=args.t,
                           max_size_mb=args.m, stream=args.stream, port=args.p, stream_width=args.sw,
                           stream_height=args.sh, stream_quality=args.sq, stream_buffer=args.sb)
    record_inst.start_recorder()
    exit_main = False
    while(not exit_main):
        try:
            sleep(1)
        except KeyboardInterrupt:
            record_inst.terminate()
            exit_main = True

if __name__ == "__main__":
    start_recording_command_line()
