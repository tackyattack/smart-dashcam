#!/usr/bin/env python
import gui
from dashRecording import DashcamRecorder
from deviceDiscovery import Discover
from time import sleep
import os
import threading
import sys
from multiprocessing import Process
from multiprocessing import Queue as ProcessQueue
from multiprocessing import Event as ProcessEvent
import Queue
import RPi.GPIO as GPIO

script_path = os.path.dirname(os.path.abspath(__file__))
record_path = os.path.join(script_path, 'recordings')
sys.path.append(os.path.join(script_path, '../Lane_Detection/src/pilanes'))
import LaneVision

# TODO: turn off streaming of localhost once you're done debugging since it's not needed

# https://picamera.readthedocs.io/en/release-1.12/fov.html
# very important to get height/width right to not kill CPU/GPU for resizing
# pay attention to camera V1 or V2
RECORD_WIDTH = 1640
RECORD_HEIGHT = 922

DEBUG_PROGRAM = False

# record interval in seconds
record_interval = 10
# maximum size of recording folder in MB
max_recording_folder_size_mb = 100
# how many of the most recent files to display
max_files_to_display = 50

# turn signal pin for lane detection
TURN_SIGNAL_PIN = 37

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
        self.terminate_event = ProcessEvent()
        self.lane_process = Process(target=self.LaneProcess)
        self.update_thread = threading.Thread(target=self.update_thread)
        self.running = True

        self.lane_process.start()
        self.update_thread.start()

    def LaneProcess(self):
        # Note: recorder and lane modules must live in the same process since the camera
        #       is shared between the two
        self.recorder = DashcamRecorder.Recorder(record_path=record_path, record_width=RECORD_WIDTH,
                                                 record_height=RECORD_HEIGHT, recording_interval_s=record_interval,
                                                 max_size_mb=max_recording_folder_size_mb, stream=True)
        self.camera = self.recorder.get_camera()
        self.recorder.start_recorder()
        self.lane_tracker = LaneVision.LaneTracker(camera=self.camera, bottom_y_boundry=0, top_y_boundry=250,
                                              transform_angle=45.0, camera_pixel_altitude=250*0.5, debug_view=True,
                                              debug_view_stage=1, log=DEBUG_PROGRAM,
                                              screen_width=480, screen_height=320)

        new_lane_alert = False
        while not self.terminate_event.is_set():
            msg = None
            try:
                msg = self.in_queue.get(block=False)
            except Queue.Empty:
                pass

            if msg is not None:
                if msg.cmd == self.LANE_CALIBRATE:
                    self.lane_tracker.calibrate()

            if self.lane_tracker.get_lane_status() and not new_lane_alert:
                new_lane_alert = True
                self.out_queue.put(ProcessMessage(self.LANE_DEPARTURE_ALERT, None))
            else:
                new_lane_alert = False

            try:
                self.terminate_event.wait(0.1)
            except KeyboardInterrupt:
                pass

        self.recorder.terminate()
        self.lane_tracker.stop()

    def terminate(self):
        self.terminate_event.set()
        self.running = False
        self.lane_process.join()
        self.update_thread.join()

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

        GPIO.setwarnings(False) # Ignore warning for now
        GPIO.setmode(GPIO.BOARD) # Use physical pin numbering
        GPIO.setup(TURN_SIGNAL_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

        self.finder = Discover.DeviceFinder()
        if not self.finder.connect(timeout=5):
            print('Failed to start device discovery')

        self.dash_gui = gui.DashcamGUI(init_callback=self.gui_init, exit_callback=self.GUI_exit_callback)
        self.dash_gui.add_event_callback('calibrate', self.calibrate_callback)
        self.dash_gui.add_event_callback('get_recordings_paths', self.get_recordings_paths_callback)
        self.dash_gui.add_event_callback('get_cameras', self.get_cameras_callback)
        self.dash_gui.add_event_callback('retrieval_mode', self.retrieval_mode_callback)
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
        gui_ready = False
        #sleep(5)
        #self.dash_gui.show_lane_warning()

        # wait for GUI to be ready
        while self.running and not self.gui_has_init:
            sleep(0.25)

        while self.running:
            # do stuff
            sleep(1)

    def lane_alert(self):
        # only show lane warning if turn signal is off
        # active low so high means the user's turn signal isn't on
        if GPIO.input(TURN_SIGNAL_PIN) == GPIO.HIGH:
            self.dash_gui.show_lane_warning()

    def gui_init(self):
        self.gui_has_init = True

    def terminate_modules(self):
        # terminate any modules here
        self.lane_process.terminate()
        self.finder.terminate()
        self.running = False

    def retrieval_mode_callback(self):
        print('mounting')

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
        cameras = self.finder.get_aux_devices(1)
        cameras.append(('tcp://127.0.0.1:8080', 'Front'))
        print(cameras)
        return cameras

# start up
main_module = MainModule()
