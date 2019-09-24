from picamera import PiCamera
from picamera import mmal, mmalobj
from time import sleep
import ctypes
import threading


ogl_cv_lib = ctypes.CDLL('../ogl_accelerator/ogl_cv.so')
ogl_cv_init_lane_tracker = ogl_cv_lib.init_lane_tracker
ogl_cv_init_lane_tracker.argtypes = None
ogl_cv_init_lane_tracker.restype = None

ogl_cv_detect_lanes_from_buffer = ogl_cv_lib.detect_lanes_from_buffer
ogl_cv_detect_lanes_from_buffer.argtypes = [ctypes.POINTER(mmal.MMAL_BUFFER_HEADER_T), ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
ogl_cv_detect_lanes_from_buffer.restype = None

data_array = (ctypes.c_char*(1920*25))()
line_data = []
with open('sample_data.txt', 'w') as del_file:
    pass
data_file = open('sample_data.txt', 'a')

camera = PiCamera()
camera.resolution = (1920, 1080)
camera.framerate = 30
print(camera._camera.outputs[2])
print(camera._splitter.outputs[2])
print(camera._splitter.outputs[2]._port[0].buffer_num)
print(camera._splitter.outputs[2]._port[0].buffer_size)
camera._splitter.outputs[2].disable()


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

process_variance = 4**2
estimated_measurement_variance = 50**2
kf_left = KalmanFilter(process_variance, estimated_measurement_variance)
kf_right = KalmanFilter(process_variance, estimated_measurement_variance)

# what percentage of lane boundary until warning is thrown
# i.e 1.0 is very sensitive, 0.0 would have to go all the way over midline
lane_tolerance = 0.75
calibrated_midline = 960
calibrated_left_boundry = 960
calibrated_right_boundry = 960

def video_callback(port, buf):
    global data_array
    global line_data
    global data_file
    global kf_left
    global kf_right

    #print("new buf from" + port.name)
    #print(buf._buf)
    addr_to_buffer = ctypes.cast(buf._buf, ctypes.c_void_p).value
    #print(addr_to_buffer)
    #print(buf.length)
    ogl_cv_detect_lanes_from_buffer(buf._buf, 1, data_array, 1)
    line_str = ''
    line_data = []
    for i in range(1920):
        line_data.append(ord(data_array[i*4]))
        line_str += str(ord(data_array[i*4])) + ', '
    left = 960
    right = 960
    max_left = 0
    max_right = 0
    for i in range(0, 960):
        right_pos = i+960
        left_pos = 960-i
        if line_data[right_pos] > max_right:
            max_right = line_data[right_pos]
            right = right_pos
        if line_data[left_pos] > max_left:
            max_left = line_data[left_pos]
            left = left_pos

    data_file.write("\n\n")
    data_file.write(line_str[:-2])
    print("left:{0} right{1}".format(left,right))
    kf_left.input_latest_noisy_measurement(left)
    kf_right.input_latest_noisy_measurement(right)
    print("actual  left:{0} right{1}".format(kf_left.get_latest_estimated_measurement(),kf_right.get_latest_estimated_measurement()))

    actual_left = kf_left.get_latest_estimated_measurement()
    actual_right = kf_right.get_latest_estimated_measurement()
    if (
        (actual_left > (calibrated_midline - lane_tolerance*(calibrated_midline - calibrated_left_boundry)))
     or (actual_right < (calibrated_midline + lane_tolerance*(calibrated_right_boundry - calibrated_midline)))
       ):
           print("---- LANE WARNING ----")

    return False # more

camera._splitter.outputs[2].params[mmal.MMAL_PARAMETER_ZERO_COPY] = True
camera._splitter.outputs[2].format = mmal.MMAL_ENCODING_OPAQUE
camera._splitter.outputs[2].commit()
camera._splitter.outputs[2].enable(video_callback)



mmalobj.print_pipeline(camera._splitter.outputs[2])
mmalobj.print_pipeline(camera._splitter.outputs[1])

#camera.start_preview()
camera.start_recording('test.h264')
#sleep(5)
#camera.stop_recording()

raw_input("Press enter to calibrate")
calibrated_midline = int((kf_left.get_latest_estimated_measurement() + kf_right.get_latest_estimated_measurement())/2)
calibrated_left_boundry = kf_left.get_latest_estimated_measurement()
calibrated_right_boundry = kf_right.get_latest_estimated_measurement()
print("***** midline: " + str(calibrated_midline))

while(1):
    pass
