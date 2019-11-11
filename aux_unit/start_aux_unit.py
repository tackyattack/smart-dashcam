#!/usr/bin/env python
from time import sleep
import os
import threading
import sys

script_path = os.path.dirname(os.path.abspath(__file__))
#record_path = os.path.join(script_path, 'recordings')
record_path = '/recordings'
sys.path.append(os.path.join(script_path, '../main_module'))
from dashRecording import DashcamRecorder
from deviceDiscovery import Discover

DEBUG_PROGRAM = False

# https://picamera.readthedocs.io/en/release-1.12/fov.html
# very important to get height/width right to not kill CPU/GPU for resizing
# pay attention to camera V1 or V2
RECORD_WIDTH = 1296
RECORD_HEIGHT = 730

STREAM_WIDTH = 480
STREAM_HEIGHT = 320

# record interval in seconds
record_interval = 30
# maximum size of recording folder in MB
max_recording_folder_size_mb = 1000

# Streaming port
stream_port = 8080


class AuxModule:

    def __init__(self):
        self.finder = Discover.DeviceFinder(stream_port)
        if not self.finder.connect(timeout=0):
            print('Failed to start device discovery')


        self.recorder = DashcamRecorder.Recorder(record_path=record_path, record_width=RECORD_WIDTH,
                                                 record_height=RECORD_HEIGHT, recording_interval_s=record_interval,
                                                 max_size_mb=max_recording_folder_size_mb, stream=True, port=stream_port,
                                                 stream_width=STREAM_WIDTH, stream_height=STREAM_HEIGHT)
        self.recorder.start_recorder()

    def terminate(self):
        self.recorder.terminate()
        self.finder.terminate()

# start up
aux_module = AuxModule()
exit_main = False

while(not exit_main):
    try:
        sleep(1)
    except KeyboardInterrupt:
        aux_module.terminate()
        exit_main = True
    except:
        aux_module.terminate()
        raise
