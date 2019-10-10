from picamera import PiCamera
from picamera import mmal, mmalobj as mo
import threading
import os
from time import sleep
import subprocess
import Queue
import sys
import io

# note: the frame rate flag for some reason has to be double the framerate
# python DashcamRecorder.py | cvlc -vvv stream:///dev/stdin --sout '#standard{access=http,mux=ts,dst=:8090}' :demux=h264 :h264-fps=40
# or you can use rtsp
# python DashcamRecorder.py | cvlc -vvv stream:///dev/stdin --sout '#rtp{sdp=rtsp://:8554/}' :demux=h264 :h264-fps=40
# You can transcode todo
# python DashcamRecorder.py | cvlc -vvv stream:///dev/stdin --sout '#transcode{vcodec=h264,vb=25,fps=20}:rtp{sdp=rtsp://:8554/}' :demux=h264 :h264-fps=40

# to play back
# omxplayer -o hdmi  rtsp://131.151.175.144:8554/

class Recorder:
    def __init__(self, record_path, recording_interval_s, max_size_mb, stream):
        self.camera = PiCamera()
        self.camera.resolution = (1640, 922)
        self.framerate = 20
        self.camera.framerate = self.framerate
        self.record_path = record_path
        self.terminate_threads = False
        self.record_interval_s = recording_interval_s
        self.recording_thread = None
        self.wrapping_thread = None
        self.current_recording_name = None
        self.wrapping_queue = Queue.Queue()
        self.max_size_mb = max_size_mb
        self.silent = False
        if not os.path.exists(self.record_path):
            os.makedirs(self.record_path)

        if stream:
            self.silent = True
            dummy_streamA = io.BytesIO()
            dummy_streamB = io.BytesIO()
            # start recording to do setup
            self.camera.start_recording(dummy_streamA, format='h264')
            # note: the resolution should be as close to the original aspect ratio as
            #       possible so that resizer doesn't have to work hard
            self.camera.start_recording(dummy_streamB, format='h264', splitter_port=3, resize=(410,320), quality=1)

            self.camera._encoders[3].encoder.outputs[0].disable()
            self.camera._encoders[3].encoder.outputs[0].enable(self.video_callback)
            self.camera.stop_recording()

            #mo.print_pipeline(self.camera._encoders[2].encoder.outputs[0])



    def get_camera(self):
        return self.camera

    def start_recorder(self):
        self.terminate_threads = False
        self.recording_thread = threading.Thread(target=self.recording_thread_func)
        self.recording_thread.start()
        self.wrapping_thread = threading.Thread(target=self.wrapping_thread_func)
        self.wrapping_thread.start()


    def terminate(self):
        self.cleanup()
        self.terminate_threads = True
        self.recording_thread = None
        self.wrapping_thread = None
        self.camera.stop_recording()

    def cleanup(self):
        for file in os.listdir(self.record_path):
            if file.endswith('.h264'):
                os.remove(os.path.join(self.record_path, file))

    def getKey(self, item):
        return item[0]

    # size in bytes
    def get_size(self, start_path = '.'):
        total_size = 0
        for dirpath, dirnames, filenames in os.walk(start_path):
            for f in filenames:
                fp = os.path.join(dirpath, f)
                # skip if it is symbolic link
                if not os.path.islink(fp):
                    total_size += os.path.getsize(fp)
        return total_size

    def get_number_file(self, file):
        # dashcam_video_0.ext
        num_str = file.split('.')[-2].split('_')[-1]
        num = int(num_str)
        return num

    def get_sorted_video_paths(self):
        file_paths = []
        for file in os.listdir(self.record_path):
            if file.endswith('.mp4') or file.endswith('.h264'):
                extracted_num = self.get_number_file(file)
                # store as tuples
                file_paths.append((extracted_num, os.path.join(self.record_path, file)))

        if len(file_paths) == 0:
            empty_paths = []
            return empty_paths

        file_paths = sorted(file_paths, key=self.getKey)
        paths_only = []
        for path in file_paths:
            paths_only.append(path[1])
        return paths_only

    def get_video_num(self):
        file_names = self.get_sorted_video_paths()
        if len(file_names) == 0:
            return 0

        most_recent_file = os.path.basename(file_names[-1])
        extracted_num = self.get_number_file(most_recent_file)

        return extracted_num + 1

    def recording_thread_func(self):
        while(not self.terminate_threads):
            record_name_path = os.path.join(self.record_path,
                                       'dashcam_video_{0}.h264'.format(self.get_video_num()))
            self.current_recording_name = record_name_path
            self.camera.start_recording(record_name_path)
            sleep_cnt = 0
            while((not self.terminate_threads) and (sleep_cnt < self.record_interval_s)):
                sleep(1.0)
                sleep_cnt = sleep_cnt + 1
            self.camera.stop_recording()
            if(not self.silent):
                print("DONE RECORDING " + record_name_path)
            self.wrapping_queue.put(record_name_path)

        if(not self.silent):
            print("recording thread closed")

    def check_reduce(self):
        size_mb = self.get_size()/1000000
        if(size_mb > self.max_size_mb):
            file_paths = []
            file_paths = self.get_sorted_video_paths()
            if len(file_paths) == 0:
                return
            # delete oldest
            file_to_delete = file_paths[0]
            if(not self.silent):
                print("removing: " + file_to_delete)
            os.remove(file_to_delete)


    def wrapping_thread_func(self):
        while(not self.terminate_threads):
            while not self.wrapping_queue.empty():
                # check if file is ready
                file_to_wrap_path = self.wrapping_queue.get()
                if(not self.silent):
                    print("WRAPPING " + file_to_wrap_path)
                # 'xxx.h264'
                file_out_path = file_to_wrap_path[:-4] + 'mp4'
                cmd_str = 'ffmpeg -framerate {0} -i {1} -c:v copy -f mp4 {2}'.format(self.framerate,
                                                                                     file_to_wrap_path,
                                                                                     file_out_path)
                cmd = cmd_str.split()
                subprocess.call(cmd, stdout=open(os.devnull, 'wb'), stderr=open(os.devnull, 'wb'))
                os.remove(file_to_wrap_path)


            self.check_reduce()
            sleep(1.0) # give the CPU some time to do other stuff

        if(not self.silent):
            print("wrapping thread closed")

    def video_callback(self, port, buf):
        if buf:
            try:
                sys.stdout.write(buf.data)
            except IOError:
                return True
        return False



if __name__ == "__main__":
    record_path = os.path.join(os.getcwd(), 'recordings')
    record_inst = Recorder(record_path=record_path, recording_interval_s=10, max_size_mb=100, stream=True)
    record_inst.start_recorder()
    exit_main = False
    while(not exit_main):
        try:
            sleep(1)
        except:
            record_inst.terminate()
            exit_main = True
