from picamera import PiCamera
from picamera import mmal, mmalobj
from time import sleep
import ctypes
import threading


# ---- OGL lane tracking library ----

ogl_cv_lib = ctypes.CDLL('../ogl_accelerator/ogl_cv.so')
ogl_cv_init_lane_tracker = ogl_cv_lib.init_lane_tracker
ogl_cv_init_lane_tracker.argtypes = [ctypes.c_char_p]
ogl_cv_init_lane_tracker.restype = None

ogl_cv_detect_lanes_from_buffer = ogl_cv_lib.detect_lanes_from_buffer
ogl_cv_detect_lanes_from_buffer.argtypes = [ctypes.POINTER(mmal.MMAL_BUFFER_HEADER_T), ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
ogl_cv_detect_lanes_from_buffer.restype = None
# -----------------------------------

class KalmanFilter(object):
    def __init__(self, process_variance, estimated_measurement_variance):
        self.process_variance = process_variance
        self.estimated_measurement_variance = estimated_measurement_variance
        self.posteri_estimate = 0.0
        self.posteri_error_estimate = 1.0

    def input_latest_noisy_measurement(self, measurement):
        priori_estimate = self.posteri_estimate
        priori_error_estimate = self.posteri_error_estimate + self.process_variance

        blending_factor = priori_error_estimate / (priori_error_estimate + self.estimated_measurement_variance)
        self.posteri_estimate = priori_estimate + blending_factor * (measurement - priori_estimate)
        self.posteri_error_estimate = (1 - blending_factor) * priori_error_estimate

    def get_latest_estimated_measurement(self):
        return self.posteri_estimate

class LaneTracker():
    # ---- NOTES ----
    # * camera must be recording for lane tracker to get mmal buffers
    # * shaders must ship with library and path must be set in init
    # ---------------
    MAX_LOG_ENTRIES = 1000
    SHADER_PATH = "/home/pi/Documents/lane_detection_exp/src/ogl_accelerator/shaders"

    def __init__(self, camera, debug_view, log):
        self.camera_instance = camera
        self.data_logging = log
        self.show_framebuffer = debug_view
        self.shutdown_lane_tracking = False
        self.process_variance = 4**2
        self.estimated_measurement_variance = 50**2
        self.kf_left = KalmanFilter(self.process_variance, self.estimated_measurement_variance)
        self.kf_right = KalmanFilter(self.process_variance, self.estimated_measurement_variance)
        # what percentage of lane boundary until warning is thrown
        # i.e 1.0 is very sensitive, 0.0 would have to go all the way over midline
        self.lane_tolerance = 0.75
        self.calibrated_midline = 960
        self.calibrated_left_boundry = 960
        self.calibrated_right_boundry = 960
        self.lane_departure = False

        self.log_count = 0

        self.new_mmal_buffer = False
        self.new_buf = None
        self.mmal_buffer_lock = threading.Lock()

        self.data_array = (ctypes.c_char*(1920*25))()
        self.line_data = []
        with open('sample_data.txt', 'w') as del_file:
            pass
        self.data_file = open('sample_data.txt', 'a')

        self.setup_camera_splitter()

        self.tracking_thread = threading.Thread(target=self.lane_tracking_thread)
        self.tracking_thread.start()

    def setup_camera_splitter(self):
        self.camera_instance._splitter.outputs[2].disable()
        self.camera_instance._splitter.outputs[2].params[mmal.MMAL_PARAMETER_ZERO_COPY] = True
        self.camera_instance._splitter.outputs[2].format = mmal.MMAL_ENCODING_OPAQUE
        self.camera_instance._splitter.outputs[2].commit()
        self.camera_instance._splitter.outputs[2].enable(self.video_callback)
        mmalobj.print_pipeline(camera._splitter.outputs[2])

    # Must be called on shutdown otherwise camera will crash
    # and you will have to reboot
    def stop(self):
        self.shutdown_lane_tracking = True

    def get_lane_stats(self):
        return self.lane_departure

    def detect_lanes(self):
        ogl_cv_detect_lanes_from_buffer(self.new_buf, 1, self.data_array, self.show_framebuffer)
        line_str = ''
        self.line_data = []
        for i in range(1920):
            self.line_data.append(ord(self.data_array[i*4]))
            line_str += str(ord(self.data_array[i*4])) + ', '
        left = 960
        right = 960
        max_left = 0
        max_right = 0
        for i in range(0, 960):
            right_pos = i+960
            left_pos = 960-i
            if self.line_data[right_pos] > max_right:
                max_right = self.line_data[right_pos]
                right = right_pos
            if self.line_data[left_pos] > max_left:
                max_left = self.line_data[left_pos]
                left = left_pos


        self.kf_left.input_latest_noisy_measurement(left)
        self.kf_right.input_latest_noisy_measurement(right)
        actual_left = self.kf_left.get_latest_estimated_measurement()
        actual_right = self.kf_right.get_latest_estimated_measurement()

        if(self.data_logging):
            if(self.log_count < self.MAX_LOG_ENTRIES):
                self.data_file.write("\n\n")
                self.data_file.write(line_str[:-2])
                self.log_count = self.log_count + 1
            print("left:{0} right{1}".format(left,right))
            print("actual  left:{0} right{1}".format(actual_left,actual_right))

        if (
            (actual_left > (self.calibrated_midline - self.lane_tolerance*(self.calibrated_midline - self.calibrated_left_boundry)))
         or (actual_right < (self.calibrated_midline + self.lane_tolerance*(self.calibrated_right_boundry - self.calibrated_midline)))
           ):
           print("---- LANE WARNING ----")
           self.lane_departure = True
        else:
            self.lane_departure = False

    def lane_tracking_thread(self):
        ogl_cv_init_lane_tracker(self.SHADER_PATH) # note: must be in same thread to keep EGL context correct
        while(not self.shutdown_lane_tracking):
            self.mmal_buffer_lock.acquire()
            if self.new_mmal_buffer:
                self.detect_lanes()
                self.new_mmal_buffer = False
            self.mmal_buffer_lock.release()
    def video_callback(self, port, buf):
        print("new buffer")
        self.mmal_buffer_lock.acquire()
        self.new_buf = buf._buf
        self.new_mmal_buffer = True
        self.mmal_buffer_lock.release()

        # wait for buffer to be consumed
        # buffer_not_done = True
        # while(buffer_not_done and not self.shutdown_lane_tracking):
        #     self.mmal_buffer_lock.acquire()
        #     if self.new_mmal_buffer == False:
        #         buffer_not_done = False
        #     self.mmal_buffer_lock.release()
        return False # expect more buffers

    def calibrate(self):
        self.calibrated_midline = int((self.kf_left.get_latest_estimated_measurement() + self.kf_right.get_latest_estimated_measurement())/2)
        self.calibrated_left_boundry = self.kf_left.get_latest_estimated_measurement()
        self.calibrated_right_boundry = self.kf_right.get_latest_estimated_measurement()
        print("***** midline: " + str(self.calibrated_midline))



camera = PiCamera()
camera.resolution = (1920, 1080)
camera.framerate = 30
#lane_tracker = LaneTracker(camera=camera, debug_view=True, log=True)

#ec = mmalobj.MMALSplitter()
#ec.outputs[0].enable()
def video_callback(port, buf):
    print("buf")
    print(buf)
    #ec.outputs[0].get_buffer()
    #print(ec)
    #input_buf.data = buf.data
    #ec.inputs[0].send_buffer(input_buf)
    sleep(5)
    return False

def encoder_output_callback(port, buf):
    print("image buf out")
    return True

def encoder_input_callback(port, buf):
    print("image buf in")
    return True


camera._splitter.outputs[2].disable()
#camera._splitter.outputs[2].params[mmal.MMAL_PARAMETER_ZERO_COPY] = True
#camera._splitter.outputs[2].format = mmal.MMAL_ENCODING_I420
camera._splitter.outputs[2].format = mmal.MMAL_ENCODING_I420
camera._splitter.outputs[2].commit()
camera._splitter.outputs[2].enable(video_callback)
mmalobj.print_pipeline(camera._splitter.outputs[2])


# ec.inputs[0].copy_from(camera._splitter.outputs[2])
# ec.inputs[0].commit()
# ec.connect(camera._splitter)
# ec.inputs[0].enable(encoder_input_callback)
# ec.outputs[0].enable(encoder_output_callback)
# mmalobj.print_pipeline(ec.outputs[0])
#
# dc = mmalobj.MMALImageDecoder()
# dc.outputs[0].format = mmal.MMAL_ENCODING_OPAQUE
# dc.outputs[0].commit()
# #dc.connect(ec)
# mmalobj.print_pipeline(dc.outputs[0])


# camera._camera.outputs[0].disable()
# camera._camera.outputs[0].params[mmal.MMAL_PARAMETER_ZERO_COPY] = True
# camera._camera.outputs[0].format = mmal.MMAL_ENCODING_OPAQUE
# camera._camera.outputs[0].commit()
# camera._camera.outputs[0].enable(image_callback)


# camera._splitter.outputs[0].enable(video_callback)

cam_lib = ctypes.CDLL('../video_test/cam.so')
create_splitter_connections = cam_lib.create_splitter_connections
create_splitter_connections.argtypes = [ctypes.POINTER(mmal.MMAL_PORT_T)]
create_splitter_connections.restype = None

while(1):
    pass

camera.start_preview()
camera.start_recording('test.h264')
#sleep(5)
#camera.stop_recording()

while(1):
    pass

exit_main = False
while(not exit_main):
    try:
        raw_input("Press enter to calibrate\n")
        lane_tracker.calibrate()
    except KeyboardInterrupt:
        lane_tracker.stop()
        exit_main = True
