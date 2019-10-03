from pilanes import LaneVision
from picamera import PiCamera
import time

camera = PiCamera()
camera.resolution = (1640, 922)
camera.framerate = 20
lane_tracker = LaneVision.LaneTracker(camera=camera, bottom_y_boundry=0, top_y_boundry=250,
                                       transform_angle=45.0, debug_view=True, debug_view_stage=8, log=True)

#camera.start_preview()
camera.start_recording('test.h264')

exit_main = False
while(not exit_main):
    try:
        raw_input("Press enter to calibrate\n")
        lane_tracker.calibrate()
    except KeyboardInterrupt:
        lane_tracker.stop()
        camera.stop_recording('test.h264')
        exit_main = True
