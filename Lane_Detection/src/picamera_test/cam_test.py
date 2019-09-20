from picamera import PiCamera
from time import sleep

camera = PiCamera()
#print(camera._camera.outputs[0])
print(camera._splitter.outputs[2])
print(camera._splitter.outputs[2]._port[0].buffer_num)
print(camera._splitter.outputs[2]._port[0].buffer_size)
