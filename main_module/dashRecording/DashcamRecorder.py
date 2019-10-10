from picamera import PiCamera
import threading
import os
from time import sleep
import subprocess
import Queue


class Recorder:
    def __init__(self, record_path, recording_interval_s, max_size_mb):
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
        if not os.path.exists(self.record_path):
            os.makedirs(self.record_path)

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
            print("DONE RECORDING " + record_name_path)
            self.wrapping_queue.put(record_name_path)

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
            print("removing: " + file_to_delete)
            os.remove(file_to_delete)


    def wrapping_thread_func(self):
        while(not self.terminate_threads):
            while not self.wrapping_queue.empty():
                # check if file is ready
                file_to_wrap_path = self.wrapping_queue.get()
                print("WRAPPING " + file_to_wrap_path)
                # 'xxx.h264'
                file_out_path = file_to_wrap_path[:-4] + 'mp4'
                cmd = 'ffmpeg -framerate {0} -i \"{1}\" -c:v copy -f mp4 \"{2}\"'.format(self.framerate,
                                                                                      file_to_wrap_path,
                                                                                         file_out_path)
                subprocess.call(cmd, shell=True)
                os.remove(file_to_wrap_path)


            self.check_reduce()
            sleep(1.0) # give the CPU some time

        print("wrapping thread closed")



if __name__ == "__main__":
    record_path = os.path.join(os.getcwd(), 'recordings')
    record_inst = Recorder(record_path=record_path, recording_interval_s=10, max_size_mb=25)
    record_inst.start_recorder()
    exit_main = False
    while(not exit_main):
        try:
            sleep(1)
        except:
            record_inst.terminate()
            exit_main = True
