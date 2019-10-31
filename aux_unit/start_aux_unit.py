#!/usr/bin/env python
from time import sleep
import os
import threading
import sys

script_path = os.path.dirname(os.path.abspath(__file__))
record_path = os.path.join(script_path, 'recordings')
sys.path.append(os.path.join(script_path, '../main_module'))
from dashRecording import DashcamRecorder

DEBUG_PROGRAM = False

# record interval in seconds
record_interval = 10
# maximum size of recording folder in MB
max_recording_folder_size_mb = 100
# how many of the most recent files to display
max_files_to_display = 50

class AuxModule:

    def __init__(self):
        self.recorder = DashcamRecorder.Recorder(record_path=record_path, recording_interval_s=record_interval,
                                                 max_size_mb=max_recording_folder_size_mb, stream=True)
        self.recorder.start_recorder()

    def terminate(self):
        self.recorder.terminate()

# start up
aux_module = AuxModule()
exit_main = False
while(not exit_main):
    try:
        sleep(1)
    except KeyboardInterrupt:
        aux_module.terminate()
        exit_main = True
