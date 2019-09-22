from picamera import PiCamera
from picamera import mmal, mmalobj
from time import sleep
import ctypes


ogl_cv_lib = ctypes.CDLL('../ogl_accelerator/ogl_cv.so')
ogl_cv_detect_lanes_from_buffer = ogl_cv_lib.detect_lanes_from_buffer
ogl_cv_detect_lanes_from_buffer.argtypes = [ctypes.POINTER(mmal.MMAL_BUFFER_HEADER_T), ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
ogl_cv_detect_lanes_from_buffer.restype = None
data_array = (ctypes.c_char*(1920*25))()
line_data = []
with open('sample_data.txt', 'w') as del_file:
    pass
data_file = open('sample_data.txt', 'a')

camera = PiCamera()
camera.resolution = (640, 360)
camera.framerate = 30
print(camera._camera.outputs[2])
print(camera._splitter.outputs[2])
print(camera._splitter.outputs[2]._port[0].buffer_num)
print(camera._splitter.outputs[2]._port[0].buffer_size)

camera._splitter.outputs[2].disable()
def video_callback(port, buf):
    global data_array
    global line_data
    global data_file
    print("new buf from" + port.name)
    print(buf._buf)
    addr_to_buffer = ctypes.cast(buf._buf, ctypes.c_void_p).value
    print(addr_to_buffer)
    print(buf.length)
    ogl_cv_detect_lanes_from_buffer(buf._buf, 1, data_array, 1)
    line_str = ''
    for i in range(1920):
        line_data.append(ord(data_array[i*4]))
        line_str += str(ord(data_array[i*4])) + ', '
    data_file.write("\n\n")
    data_file.write(line_str[:-2])
    #print(line_data)
    return False # more

camera._splitter.outputs[2].params[mmal.MMAL_PARAMETER_ZERO_COPY] = True
camera._splitter.outputs[2].format = mmal.MMAL_ENCODING_OPAQUE
camera._splitter.outputs[2].commit()
camera._splitter.outputs[2].enable(video_callback)



mmalobj.print_pipeline(camera._splitter.outputs[2])
mmalobj.print_pipeline(camera._splitter.outputs[1])

camera.start_recording('test.h264')
#sleep(5)
#camera.stop_recording()
while(1):
    pass
