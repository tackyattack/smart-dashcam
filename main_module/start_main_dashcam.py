#!/usr/bin/env python
import gui
from dashRecording import DashcamRecorder
from time import sleep
import os
import threading

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
                                                 max_size_mb=max_recording_folder_size_mb, stream=True)

        self.dash_gui = gui.DashcamGUI(init_callback=self.gui_init, exit_callback=self.GUI_exit_callback)
        self.dash_gui.add_event_callback('calibrate', self.calibrate_callback)
        self.dash_gui.add_event_callback('get_recordings_paths', self.get_recordings_paths_callback)
        self.dash_gui.add_event_callback('get_cameras', self.get_cameras_callback)
        self.gui_has_init = False

        self.app_thread = threading.Thread(target=self.application_thread)
        self.app_thread.start()
        self.running = True

        # Tkinter isn't thread safe, so it must be in the main thread
        # therefore, start is blocking and workers should be done here in their own threads
        try:
            self.dash_gui.start()
        except:
            self.terminate_modules()
            raise

    def application_thread(self):
        self.recorder.start_recorder()
        gui_ready = False
        # wait for GUI to be ready
        while self.running and not self.gui_has_init:
            sleep(0.25)
        sleep(8)
        self.dash_gui.show_lane_warning()
        while self.running:
            sleep(1)

    def gui_init(self):
        self.gui_has_init = True

    def terminate_modules(self):
        self.running = False
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
        return [('tcp://192.168.0.152:8080', 'camera 1'), ('tcp://131.151.175.144:8080', 'camera 2')]

# start up
main_module = MainModule()
