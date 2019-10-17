from picamera import PiCamera
from picamera import mmal, mmalobj
import BirdsEye as bird
from time import sleep
import ctypes
import threading
import os

root_path = os.path.dirname(os.path.realpath(__file__))


# ---- OGL lane tracking library ----
ogl_cv_lib = ctypes.CDLL(os.path.join(root_path, 'ogl_cv.so'))
ogl_cv_init_lane_tracker = ogl_cv_lib.init_lane_tracker
ogl_cv_init_lane_tracker.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
ogl_cv_init_lane_tracker.restype = None

# void detect_lanes_from_buffer(int download, char *mem_ptr, int bottom_y_boundry, int top_y_boundry, float angle, float *transformation_matrix, int show, int stage_to_show);
ogl_cv_detect_lanes_from_buffer = ogl_cv_lib.detect_lanes_from_buffer
ogl_cv_detect_lanes_from_buffer.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.POINTER(ctypes.c_float), ctypes.c_int, ctypes.c_int]
ogl_cv_detect_lanes_from_buffer.restype = None

load_egl_image_from_buffer = ogl_cv_lib.load_egl_image_from_buffer
load_egl_image_from_buffer.argtypes = [ctypes.POINTER(mmal.MMAL_BUFFER_HEADER_T)]
load_egl_image_from_buffer.restype = None

shutdown_lane_tracker = ogl_cv_lib.shutdown_lane_tracker
shutdown_lane_tracker.argtypes = None
shutdown_lane_tracker.restype = None
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
    SHADER_PATH = os.path.join(root_path, 'shaders')
    print("loading shaders from: {0}".format(SHADER_PATH))

    def __init__(self, camera, bottom_y_boundry, top_y_boundry, transform_angle, camera_pixel_altitude, debug_view, debug_view_stage, log, screen_width, screen_height):
        self.camera_instance = camera
        self.data_logging = log
        self.show_framebuffer = debug_view
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.stage_to_show = debug_view_stage
        self.shutdown_lane_tracking = False
        self.process_variance = 4**2
        self.estimated_measurement_variance = 50**2
        self.bottom_y_boundry = bottom_y_boundry
        self.top_y_boundry = top_y_boundry

        self.transform_angle = transform_angle
        self.transform_matrix = (ctypes.c_float*(3*3))()
        bm = bird.BirdsEyeMath()
        pixel_height = top_y_boundry - bottom_y_boundry
        self.transform_matrix = bm.numpy_to_float_mat(bm.get_transformation_matrix(width=1024, height=pixel_height, angle=self.transform_angle,
                                                                                   camera_pixel_altitude=camera_pixel_altitude), 3*3)

        self.kf_left = KalmanFilter(self.process_variance, self.estimated_measurement_variance)
        self.kf_right = KalmanFilter(self.process_variance, self.estimated_measurement_variance)
        # what percentage of lane boundary until warning is thrown
        # i.e 1.0 is very sensitive, 0.0 would have to go all the way over midline
        self.lane_tolerance = 0.75
        self.calibrated_midline = 512
        self.calibrated_left_boundry = 512
        self.calibrated_right_boundry = 512
        self.lane_departure = False

        self.log_count = 0

        self.new_buf = None
        self.mmal_buf_header = None
        self.mmal_buffer_lock = threading.Lock()
        self.mmal_consume_lock = threading.Lock()

        self.data_array = (ctypes.c_char*(1024*25))()
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
        mmalobj.print_pipeline(self.camera_instance._splitter.outputs[2])

    # Must be called on shutdown otherwise camera will crash
    # and you will have to reboot
    def stop(self):
        self.shutdown_lane_tracking = True

    def get_lane_stats(self):
        return self.lane_departure

    def LR_peak_finder(self, lane_data, middle_point, edge_buffer):
        left = middle_point
        right = middle_point

        # don't go all the way to the edges
        # since sobel could be picking up edges of picture
        extender_size = middle_point - edge_buffer
        max_left = 0
        max_right = 0
        left_avg = 0.0
        right_avg = 0.0

        # find LR max, and averages for noise floor
        for i in range(0, extender_size):
            right_pos = i+middle_point
            left_pos = middle_point-i
            left_avg = left_avg + lane_data[left_pos]
            right_avg = right_avg + lane_data[right_pos]
            if lane_data[right_pos] > max_right:
                max_right = lane_data[right_pos]
            if lane_data[left_pos] > max_left:
                max_left = lane_data[left_pos]


        left_avg = left_avg*1.0/extender_size
        right_avg = right_avg*1.0/extender_size
        left_threshold  = (max_left+left_avg)/2.0
        right_threshold = (max_right+right_avg)/2.0

        # now feel forward for the first peak on L and R sides
        for i in range(0, extender_size):
            right_pos = i+middle_point
            if lane_data[right_pos] > right_threshold:
                right = right_pos
                break
        for i in range(0, extender_size):
            left_pos = middle_point-i
            if lane_data[left_pos] > left_threshold:
                left = left_pos
                break

        if(self.data_logging):
            print("L threshold: {0}  R threshold: {1}".format(left_threshold,right_threshold))

        return left, right


    def detect_lanes(self):
        ogl_cv_detect_lanes_from_buffer(1, self.data_array, self.bottom_y_boundry, self.top_y_boundry, self.transform_angle, self.transform_matrix, self.show_framebuffer, self.stage_to_show)
        line_str = ''
        self.line_data = []
        for i in range(1024):
            self.line_data.append(ord(self.data_array[i*4]))
            line_str += str(ord(self.data_array[i*4])) + ', '


        left, right = self.LR_peak_finder(lane_data=self.line_data, middle_point=512, edge_buffer=60)

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
        ogl_cv_init_lane_tracker(self.SHADER_PATH, self.screen_width, self.screen_height) # note: must be in same thread to keep EGL context correct
        while(not self.shutdown_lane_tracking):
            #print("acquiring buffer")
            self.mmal_consume_lock.acquire()
            #print("I'm holding the buffer producer now")

            # wait for new buffer to be produced
            done = False
            while(not done):
                self.mmal_buffer_lock.acquire()
                if(self.new_buf==True):
                    done = True
                self.mmal_buffer_lock.release()
            #print(self.new_buf)
            # load it in
            #print("buffer loading into image")
            load_egl_image_from_buffer(self.mmal_buf_header)
            #sleep(3)
            self.mmal_consume_lock.release()
            #print("detecting lanes")
            # now take as much time as needed to process the image (won't block producer)
            self.detect_lanes()
            #sleep(5)
        shutdown_lane_tracker()

    def video_callback(self, port, buf):
        #print("new buffer")

        self.mmal_buffer_lock.acquire()
        self.mmal_buf_header = buf._buf
        self.new_buf = True
        self.mmal_buffer_lock.release()

        self.mmal_consume_lock.acquire()
        self.mmal_consume_lock.release()

        self.new_buf = False

        return False # expect more buffers

    def calibrate(self):
        self.calibrated_midline = int((self.kf_left.get_latest_estimated_measurement() + self.kf_right.get_latest_estimated_measurement())/2)
        self.calibrated_left_boundry = self.kf_left.get_latest_estimated_measurement()
        self.calibrated_right_boundry = self.kf_right.get_latest_estimated_measurement()
        print("***** midline: " + str(self.calibrated_midline))


if __name__ == "__main__":
    camera = PiCamera()
    camera.resolution = (1640, 922)
    camera.framerate = 20
    lane_tracker = LaneTracker(camera=camera, bottom_y_boundry=0, top_y_boundry=250,
                               transform_angle=45.0, debug_view=True, debug_view_stage=8, log=True, screen_width=480, screen_height=320)

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
