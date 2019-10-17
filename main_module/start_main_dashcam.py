#!/usr/bin/env python
import gui
from time import sleep

exit_main = False

def GUI_exit_callback():
    global exit_main
    exit_main = True

dash_gui = gui.DashcamGUI(GUI_exit_callback)

def main():
    while not exit_main:
        sleep(1.0) # don't need the sleep -- just here to not consume a bunch of CPU time

while not exit_main:
    try:
        main()
    except KeyboardInterrupt:
        dash_gui.terminate()
        exit_main = True
