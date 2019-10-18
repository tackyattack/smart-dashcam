#!/usr/bin/env python
import gui
from time import sleep

def GUI_exit_callback():
    print('gui exited')

def calibrate_callback():
    print('calibrate pressed')

dash_gui = gui.DashcamGUI(GUI_exit_callback)
dash_gui.add_event_callback('calibrate', calibrate_callback)

dash_gui.show_lane_warning()

# Tkinter isn't thread safe, so it must be in the main thread
# therefore, start is blocking and workers should be done here in their own threads
dash_gui.start()
