#!/usr/bin/env python
import gui
from dashRecording import DashcamRecorder
from time import sleep
import os

# record interval in seconds
record_interval = 10
# maximum size of recording folder in MB
max_recording_folder_size_mb = 100
# how many of the most recent files to display
max_files_to_display = 50


script_path = os.path.dirname(os.path.abspath(__file__))
record_path = os.path.join(script_path, 'recordings')

#dash_gui.show_lane_warning()

class MainModule:

    def __init__(self):
        self.recorder = DashcamRecorder.Recorder(record_path=record_path, recording_interval_s=record_interval,
                                                 max_size_mb=max_recording_folder_size_mb, stream=False)

        self.dash_gui = gui.DashcamGUI(exit_callback=self.GUI_exit_callback)
        self.dash_gui.add_event_callback('calibrate', self.calibrate_callback)
        self.dash_gui.add_event_callback('get_recordings_paths', self.get_recordings_paths_callback)
        self.dash_gui.add_event_callback('get_cameras', self.get_cameras_callback)

        self.recorder.start_recorder()

        # Tkinter isn't thread safe, so it must be in the main thread
        # therefore, start is blocking and workers should be done here in their own threads
        try:
            self.dash_gui.start()
        except:
            self.terminate_modules()
            raise

    def terminate_modules(self):
        self.recorder.terminate()

    def GUI_exit_callback(self):
        self.terminate_modules()
        print('gui exited')

    def calibrate_callback(self):
        print('calibrate pressed')

    def get_recordings_paths_callback(self):
        sorted_paths = self.recorder.get_sorted_video_paths()
        paths = []
        top_index = len(sorted_paths)-1
        bottom_index = top_index - max_files_to_display - 1
        if bottom_index < -1:
            bottom_index = -1
        for i in range(top_index, bottom_index, -1):
            current_path = sorted_paths[i]
            filename, extension = os.path.splitext(current_path)
            if('mp4' in extension):
                paths.append(current_path)
        return paths

    def get_cameras_callback(self):
        return [('rtsp://131.151.175.144:8554/', 'camera 1'), ('rtsp://131.151.175.144:8554/', 'camera 2')]

# start up
main_module = MainModule()
