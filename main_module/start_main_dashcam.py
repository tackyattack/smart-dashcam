#!/usr/bin/env python
import gui
from time import sleep

def GUI_exit_callback():
    print('gui exited')

def calibrate_callback():
    print('calibrate pressed')

def get_recordings_paths_callback():
    return ['/home/pi/Documents/smart-dashcam/main_module/example_recordings/dashcam_video_0.mp4',
            '/home/pi/Documents/smart-dashcam/main_module/example_recordings/dashcam_video_1.mp4']

def get_cameras_callback():
    return [('rtsp://131.151.175.144:8554/', 'camera 1'), ('rtsp://131.151.175.144:8554/', 'camera 2')]

dash_gui = gui.DashcamGUI(exit_callback=GUI_exit_callback)
dash_gui.add_event_callback('calibrate', calibrate_callback)
dash_gui.add_event_callback('get_recordings_paths', get_recordings_paths_callback)
dash_gui.add_event_callback('get_cameras', get_cameras_callback)

#dash_gui.show_lane_warning()

# Tkinter isn't thread safe, so it must be in the main thread
# therefore, start is blocking and workers should be done here in their own threads
dash_gui.start()
