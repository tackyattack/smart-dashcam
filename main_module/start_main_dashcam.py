#!/usr/bin/env python
import gui
from time import sleep

def GUI_exit_callback():
    pass

def calibrate_callback():
    print('calibrate pressed')

dash_gui = gui.DashcamGUI(GUI_exit_callback)
dash_gui.add_event_callback('Calibrate', calibrate_callback)

# Tkinter isn't thread safe, so it must be in the main thread
# therefore, start is blocking and workers should be done here in their own threads
dash_gui.start()
