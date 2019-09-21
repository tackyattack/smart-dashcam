from picamera import PiCamera
from picamera import mmal, mmalobj
from time import sleep
import ctypes

picam_tex_lib = ctypes.CDLL('../picamera_to_tex/picam_tex.so')
picam_tex_test = picam_tex_lib.test_func
picam_tex_test.argtypes = [ctypes.POINTER(mmal.MMAL_BUFFER_HEADER_T)]
picam_tex_test.restype = None

camera = PiCamera()
camera.resolution = (640, 360)
camera.framerate = 30
print(camera._camera.outputs[2])
print(camera._splitter.outputs[2])
print(camera._splitter.outputs[2]._port[0].buffer_num)
print(camera._splitter.outputs[2]._port[0].buffer_size)

camera._splitter.outputs[2].disable()
def video_callback(port, buf):
    print("new buf from" + port.name)
    #print(len(buf.data))
    #buf.release()
    print(buf._buf)
    addr_to_buffer = ctypes.cast(buf._buf, ctypes.c_void_p).value
    print(addr_to_buffer)
    picam_tex_test(buf._buf)
    #while(1):
    #    pass
    #sleep(2.0)
    return False # more
    #return bool(buf.flags & mmal.MMAL_BUFFER_HEADER_FLAG_FRAME_END)

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

#camera._camera.outputs[1].params[mmal.MMAL_PARAMETER_CAPTURE] = True
#camera._camera.outputs[1].params[mmal.MMAL_PARAMETER_CAPTURE] = False
#camera._splitter.outputs[2].params[mmal.MMAL_PARAMETER_CAPTURE] = False
#camera._splitter.outputs[2].params[mmal.MMAL_PARAMETER_CAPTURE] = False
#print(camera._splitter.outputs[2].params)
