#!/usr/bin/env python
import gui
from dashRecording import DashcamRecorder
from time import sleep
import os
import threading
import sys
from multiprocessing import Process
from multiprocessing import Queue as ProcessQueue
import Queue

script_path = os.path.dirname(os.path.abspath(__file__))
record_path = os.path.join(script_path, 'recordings')
sys.path.append(os.path.join(script_path, '../Lane_Detection/src/pilanes'))
import LaneVision

# TODO: turn off streaming of localhost once you're done debugging since it's not needed

DEBUG_PROGRAM = False

# record interval in seconds
record_interval = 10
# maximum size of recording folder in MB
max_recording_folder_size_mb = 100
# how many of the most recent files to display
max_files_to_display = 50


#dash_gui.show_lane_warning()

class ProcessMessage:
    def __init__(self, cmd, data):
        self.cmd = cmd
        self.data = data

class LaneDetectionProcess:
    TERMINATE_LANE_TRACKING = 1
    LANE_DEPARTURE_ALERT = 2
    LANE_CALIBRATE = 3

    def __init__(self, lane_departure_callback):

        self.lane_departure_callback = lane_departure_callback
        self.running = False
        self.out_queue = ProcessQueue()
        self.in_queue = ProcessQueue()
        p = Process(target=self.LaneProcess)
        t = threading.Thread(target=self.update_thread)
        self.running = True

        p.start()
        t.start()

    def LaneProcess(self):
        # Note: recorder and lane modules must live in the same process since the camera
        #       is shared between the two
        self.recorder = DashcamRecorder.Recorder(record_path=record_path, recording_interval_s=record_interval,
                                                 max_size_mb=max_recording_folder_size_mb, stream=True)
        self.camera = self.recorder.get_camera()
        self.recorder.start_recorder()
        self.lane_tracker = LaneVision.LaneTracker(camera=self.camera, bottom_y_boundry=0, top_y_boundry=250,
                                              transform_angle=45.0, camera_pixel_altitude=250*0.5, debug_view=False,
                                              debug_view_stage=1, log=DEBUG_PROGRAM,
                                              screen_width=480, screen_height=320)

        terminate_process = False
        new_lane_alert = False
        while not terminate_process:
            msg = None
            try:
                msg = self.in_queue.get(block=False)
            except Queue.Empty:
                pass

            if msg is not None:
                if msg.cmd == self.TERMINATE_LANE_TRACKING:
                    terminate_process = True
                if msg.cmd == self.LANE_CALIBRATE:
                    self.lane_tracker.calibrate()

            if self.lane_tracker.get_lane_status() and not new_lane_alert:
                new_lane_alert = True
                self.out_queue.put(ProcessMessage(self.LANE_DEPARTURE_ALERT, None))
            else:
                new_lane_alert = False
            try:
                sleep(0.1)
            except KeyboardInterrupt:
                terminate_process = True

        self.recorder.terminate()
        self.lane_tracker.stop()

    def terminate(self):
        self.in_queue.put(ProcessMessage(self.TERMINATE_LANE_TRACKING, None))
        self.running = False

    def calibrate(self):
        self.in_queue.put(ProcessMessage(self.LANE_CALIBRATE, None))

    def update_thread(self):
        while self.running:
            msg = None
            try:
                msg = self.out_queue.get(block=False)
            except Queue.Empty:
                pass

            if msg is not None:
                if msg.cmd == self.LANE_DEPARTURE_ALERT:
                    self.lane_departure_callback()
            sleep(0.1)


class MainModule:

    def __init__(self):


        self.dash_gui = gui.DashcamGUI(init_callback=self.gui_init, exit_callback=self.GUI_exit_callback)
        self.dash_gui.add_event_callback('calibrate', self.calibrate_callback)
        self.dash_gui.add_event_callback('get_recordings_paths', self.get_recordings_paths_callback)
        self.dash_gui.add_event_callback('get_cameras', self.get_cameras_callback)
        self.gui_has_init = False

        self.app_thread = threading.Thread(target=self.application_thread)
        self.running = True

        self.app_thread.start()
        self.lane_process = LaneDetectionProcess(self.lane_alert)

        # Tkinter isn't thread safe, so it must be in the main thread
        # therefore, start is blocking and workers should be done here in their own threads
        try:
            self.dash_gui.start()
        except:
            self.terminate_modules()
            raise

    def application_thread(self):
        #lane_process = LaneDetectionProcess(self.lane_alert)
        gui_ready = False

        # wait for GUI to be ready
        while self.running and not self.gui_has_init:
            sleep(0.25)

        while self.running:
            # do stuff
            sleep(1)

    def lane_alert(self):
        #self.dash_gui.show_lane_warning()
        pass

    def gui_init(self):
        self.gui_has_init = True

    def terminate_modules(self):
        # terminate any modules here
        self.lane_process.terminate()
        self.running = False


    def GUI_exit_callback(self):
        self.terminate_modules()
        print('gui exited')

    def calibrate_callback(self):
        self.lane_process.calibrate()
        print('calibrate pressed')

    def get_recordings_paths_callback(self):
        sorted_paths = DashcamRecorder.get_sorted_video_paths(record_path)
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
