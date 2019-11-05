import sys
import os
import threading
import tkinter as tk
import pprint
import Queue
from time import sleep
import pygame
import subprocess
import signal


if sys.version_info[0] >= 3:
    import PySimpleGUI as sg
else:
    import PySimpleGUI27 as sg

registered_callbacks = {
'calibrate': None,
'exit_gui': None,
'gui_has_init': None,
'get_recordings_paths': None,
'get_cameras': None,
'retrieval_mode': None
}


FULL_SCREEN_WIDTH = 482
FULL_SCREEN_HEIGHT = 322
TOP_BAR_OFFSET = -30
LEFT_OFFSET = -2


# command queue for windows
WINDOW_COMMAND_POP_VIEW = 1
WINDOW_COMMAND_PUSH_VIEW = 2
window_command_queue = Queue.Queue()
def put_window_command(command, window_data):
    window_command_queue.put((command, window_data))

class window_push_packet:
    def __init__(self, view_class, data):
        self.view_class = view_class
        self.data = data

class window_pop_packet:
    def __init__(self, view_instance):
        self.view_instance = view_instance

class video_player_packet:
    def __init__(self, video_path, stream):
        self.video_path = video_path
        self.stream = stream

def get_script_path():
    return os.path.dirname(os.path.abspath(__file__))


def get_pid(name):
    try:
        pid_str = subprocess.check_output(['pidof',name])
        pid_str = pid_str.split()[0]
        if len(pid_str) > 0:
            return int(pid_str)
        else:
            return -1
    except OSError:
        return -1
    except subprocess.CalledProcessError:
        return -1

class GUIView(object):

    def __init__(self):
        blank_layout = [[]]

        self.view_window = sg.Window('GUI View', blank_layout, no_titlebar=False, location=(LEFT_OFFSET,TOP_BAR_OFFSET),
                           size=(FULL_SCREEN_WIDTH,FULL_SCREEN_HEIGHT),
                            grab_anywhere=False, keep_on_top=True, background_color='black')
        self.view_window.Finalize()
        self.view_window_root = self.view_window.TKroot
        self.view_frame = tk.Frame(self.view_window_root, width = FULL_SCREEN_WIDTH, height=FULL_SCREEN_HEIGHT, bg='black')
        self.view_frame.pack_propagate(False)
        self.view_frame.pack()
        self.view_window_root.config(cursor="none")
        self.visible = False

    def close_view(self):
        self.view_window.Close()

    # visible/not visible can be used for behavior that should happen when a view
    # gets covered/uncovered

    def is_visible(self):
        self.visible = True

    def is_not_visible(self):
        self.visible = False

    def update(self):
        pass


class GUIListView(GUIView):
    def __init__(self, list_items):
        super(GUIListView, self).__init__()

        bf = ('Helvetica', '15', 'bold')
        width=22
        height=1
        tk.Button(self.view_frame, text='Back',
                 bg='red', activebackground='red',activeforeground='white', fg='white',
                 font=bf,
                 width=width, height=height, command=self.exit_callback).pack()

        listbox = tk.Listbox(self.view_frame, width=23, font=("Helvetica", 25))

        for list_item in list_items:
            listbox.insert(tk.END, list_item)
        listbox.pack(side="left", fill="both")

        scrollbar = tk.Scrollbar(self.view_frame, width=30,orient="vertical")
        scrollbar.config(command=listbox.yview)
        scrollbar.pack(side="left", fill="both")
        listbox.config(yscrollcommand=scrollbar.set)
        listbox.bind('<<ListboxSelect>>', self.list_press_callback)

    def list_press_callback(self, event):
        w = event.widget
        index = int(w.curselection()[0])
        value = w.get(index)
        print 'You selected item %d: "%s"' % (index, value)
        put_window_command(WINDOW_COMMAND_POP_VIEW, None)

    def exit_callback(self):
        put_window_command(WINDOW_COMMAND_POP_VIEW, None)

class GUICameraListView(GUIListView):
    def __init__(self, list_items):
        self.items = []
        self.list_items = list_items
        for path, filename in list_items:
            self.items.append(filename)
        super(GUICameraListView, self).__init__(self.items)

    def list_press_callback(self, event):
        w = event.widget
        index = int(w.curselection()[0])
        value = w.get(index)
        print 'You selected item %d: "%s"' % (index, value)
        video_path = self.list_items[index][0]
        put_window_command(WINDOW_COMMAND_PUSH_VIEW,
                           window_push_packet(view_class=GUIVideoPlayer,
                           data=video_player_packet(video_path=video_path, stream=True)))

class GUIRecordingsListView(GUIListView):
    def __init__(self, list_items):
        self.items = []
        self.list_items = list_items
        for path, filename in list_items:
            self.items.append(filename)
        super(GUIRecordingsListView, self).__init__(self.items)

    def list_press_callback(self, event):
        w = event.widget
        index = int(w.curselection()[0])
        value = w.get(index)
        print 'You selected item %d: "%s"' % (index, value)
        video_path = self.list_items[index][0]
        put_window_command(WINDOW_COMMAND_PUSH_VIEW,
                           window_push_packet(view_class=GUIVideoPlayer,
                           data=video_player_packet(video_path=video_path, stream=False)))

class GUILaneWarningView(GUIView):
    def __init__(self):
        super(GUILaneWarningView, self).__init__()
        tk.Label(self.view_frame, text="WARNING", font=('Helvetica', '50', 'bold'), bg='black', fg='red', pady=40).pack()
        tk.Label(self.view_frame, text="LANE DRIFT", font=('Helvetica', '50', 'bold'), bg='black', fg='yellow').pack()

    def update(self):
        self.view_frame.pack_forget()
        self.view_frame.pack()

class GUIVideoPlayer(GUIView):
    def __init__(self, video_path, stream):
        super(GUIVideoPlayer, self).__init__()
        print('playing: ' + video_path)
        self.video_path = video_path
        self.stream = stream
        self.process = None
        self.running = False
        self.player = None
        self.covered = False
        self.ready_to_exit = False
        tk.Button(self.view_frame, text='',
                 bg='white', activebackground='white',activeforeground='white', fg='white',
                 width=100, height=100, command=self.exit_callback).pack()

        x = threading.Thread(target=self.start_player)
        x.start()

    def kill_player(self, process_name):
        full_name = process_name
        name_only = process_name.split('.')[0]
        pid = get_pid(full_name)
        while pid != -1:
            pid = get_pid(full_name)
            if(pid != -1):
                subprocess.call(['pkill', name_only])
            sleep(0.5)
        self.player = None

    def wait_for_process_open(self, process_name):
        timeout_cnt = 0
        timeout = 20
        pid = get_pid(process_name)
        while (pid == -1) and (timeout_cnt < timeout):
            pid = get_pid(process_name)
            sleep(1)
            timeout_cnt = timeout_cnt + 1

        if timeout_cnt == timeout:
            return False
        return True

    def start_player(self):
        omxplayer_location = 'omxplayer.bin'
        omxplayer = 'omxplayer.bin'
        dash_stream_location = os.path.join(get_script_path(), 'dashRecording/Stream/dashcam_streamer/dash_stream.bin')
        dash_stream = 'dash_stream.bin'

        self.player = None
        if self.stream:
            self.player = dash_stream
        else:
            self.player = omxplayer

        if self.stream:
            uri = os.path.basename(self.video_path)
            print(uri)
            stream_ip = uri.split(':')[0]
            stream_port = uri.split(':')[1]
            cmd = '{0} {1} {2}'.format(dash_stream_location, stream_ip, stream_port)
            cmd = cmd.split()
        else:
            cmd = '{0} -o hdmi {1}'.format(omxplayer_location, self.video_path)
            cmd = cmd.split()

        self.process = subprocess.Popen(cmd, stdin=subprocess.PIPE)
        self.running = True

        # wait for it to open
        self.wait_for_process_open(self.player)

        # wait for either exit button or video end, or it to be covered by another view
        while (self.running) and (get_pid(self.player) != -1):
            sleep(0.5)

        if self.process.poll() is not None:
            try:
                self.process.terminate()
            except OSError:
                pass

        self.kill_player(self.player)
        # since this is not done by the user, it is best to pop this specific view
        # since it might not be visible and could pop the lane warning, for example
        put_window_command(WINDOW_COMMAND_POP_VIEW, window_pop_packet(view_instance=self))

    def exit_callback(self):
        self.running = False

    def is_not_visible(self):
        self.covered = True

    def is_visible(self):
        self.covered = False


class GUISettingsView(GUIView):
    def __init__(self):
        super(GUISettingsView, self).__init__()
        bf = ('Helvetica', '15', 'bold')
        width=22
        height=1
        tk.Button(self.view_frame, text='Back',
                 bg='red', activebackground='red',activeforeground='white', fg='white',
                 font=bf,
                 width=width, height=height, command=self.exit_callback).pack()

        tk.Button(self.view_frame, text='Recordings Retrieval Mode',
                 bg='green', activebackground='green',activeforeground='white', fg='white',
                 font=bf,
                 width=30, height=2, command=self.retrieval_mode_pressed).pack(pady=(50, 0))

    def retrieval_mode_pressed(self):
        print('mount pressed')
        registered_callbacks['retrieval_mode']()

    def exit_callback(self):
        put_window_command(WINDOW_COMMAND_POP_VIEW, None)


class GUIMainView(GUIView):
    def __init__(self):
        super(GUIMainView, self).__init__()
        bf = ('Helvetica', '15', 'bold')
        width=18
        height=5
        tk.Button(self.view_frame, text='Lane Calibrate',
                 bg='red', activebackground='red',activeforeground='white', fg='white',
                 font=bf,
                 width=width, height=height, command=self.calibrate_callback).grid(row=0, column=0)
        tk.Button(self.view_frame, text='Play Recording',
                 bg='blue', activebackground='blue', activeforeground='white',fg='white',
                 font=bf,
                 width=width, height=height, command=self.play_recording_callback).grid(row=0, column=1)
        tk.Button(self.view_frame, text='View Camera',
                 bg='green', activebackground='green', activeforeground='white',fg='white',
                 font=bf,
                 width=width, height=height, command=self.view_camera_callback).grid(row=1, column=0)
        tk.Button(self.view_frame, text='Settings',
                 bg='orange', activebackground='orange', activeforeground='white',fg='white',
                 font=bf,
                 width=width, height=height, command=self.settings_callback).grid(row=1, column=1)

    def calibrate_callback(self):
        registered_callbacks['calibrate']()

    def play_recording_callback(self):
        put_window_command(WINDOW_COMMAND_PUSH_VIEW, window_push_packet(view_class=GUIRecordingsListView, data=None))

    def view_camera_callback(self):
        put_window_command(WINDOW_COMMAND_PUSH_VIEW, window_push_packet(view_class=GUICameraListView, data=None))

    def settings_callback(self):
        put_window_command(WINDOW_COMMAND_PUSH_VIEW, window_push_packet(view_class=GUISettingsView, data=None))

    def is_visible(self):
        registered_callbacks['gui_has_init']()



class DashcamGUI:

    def __init__(self, init_callback, exit_callback):

        self.add_event_callback('exit_gui', exit_callback)
        self.add_event_callback('gui_has_init', init_callback)
        self.windows_view_stack = []
        self.recordings_files = [('/', 'no files')]

        self.window = None
        self.running = True
        self.pg_mixer = pygame.mixer
        self.pg_mixer.init()
        self.lane_warning_open = False


    def start(self):
        try:
            self.event_thread()
        except KeyboardInterrupt:
            self.terminate()

    def terminate(self):
        self.running = False
        registered_callbacks['exit_gui']()

    def add_event_callback(self, event_name, callback_function):
        if event_name in registered_callbacks:
            registered_callbacks[event_name] = callback_function
        else:
            raise Exception('callback not defined')

    def setup_GUI(self):
        put_window_command(WINDOW_COMMAND_PUSH_VIEW, window_push_packet(view_class=GUIMainView, data=None))

    def close_lane_warning(self):
        sleep(5)
        put_window_command(WINDOW_COMMAND_POP_VIEW, None)
        self.lane_warning_open = False

    def show_lane_warning(self):
        if not self.lane_warning_open:
            self.lane_warning_open = True
            put_window_command(WINDOW_COMMAND_PUSH_VIEW, window_push_packet(view_class=GUILaneWarningView, data=None))
            beep_path = os.path.join(get_script_path(), 'lane_beep.wav')
            self.pg_mixer.music.load(beep_path)
            self.pg_mixer.music.play(0)
            x = threading.Thread(target=self.close_lane_warning)
            x.start()

    def event_thread(self):
        self.setup_GUI()

        while self.running:
            while not window_command_queue.empty():
                window_command = window_command_queue.get(block=True)
                if window_command[0] == WINDOW_COMMAND_POP_VIEW:
                    pop_index = None
                    view_stack_sz = len(self.windows_view_stack)
                    # if it wants to pop a speific view instance, find it
                    if window_command[1] is not None:
                        for i in range(view_stack_sz):
                            if window_command[1].view_instance is self.windows_view_stack[i]:
                                pop_index = i
                    else:
                        pop_index = view_stack_sz - 1

                    if pop_index is not None:
                        self.windows_view_stack[pop_index].close_view()
                        del self.windows_view_stack[pop_index]
                        # if a top view just got uncovered, tell it
                        if (len(self.windows_view_stack) > 0) and (pop_index == view_stack_sz-1):
                            self.windows_view_stack[-1].is_visible()
                    else:
                        print("error: pop index not found")

                if window_command[0] == WINDOW_COMMAND_PUSH_VIEW:
                    new_view = None
                    if window_command[1].view_class == GUIMainView:
                        new_view = self.main_view_view_setup()
                    if window_command[1].view_class == GUIRecordingsListView:
                        new_view = self.view_files_view_setup()
                    if window_command[1].view_class == GUICameraListView:
                        new_view = self.view_camera_view_setup()
                    if window_command[1].view_class == GUILaneWarningView:
                        new_view = self.lane_warning_view_setup()
                    if window_command[1].view_class == GUIVideoPlayer:
                        new_view = self.video_player_view_setup(window_command[1].data)
                    if window_command[1].view_class == GUISettingsView:
                        new_view = self.settings_view_setup()

                    # if there's a view that's about to get covered, tell it
                    if(len(self.windows_view_stack) > 1):
                        self.windows_view_stack[-1].is_not_visible()
                    self.windows_view_stack.append(new_view)
                    new_view.is_visible()

            if(len(self.windows_view_stack) > 0):
                # read only top of view stack since that's the only visible one
                event, values = self.windows_view_stack[-1].view_window.Read(timeout=100)
                # window closed (through exit icon -- so we probably won't need this)
                if event is None:
                    put_window_command(WINDOW_COMMAND_POP_VIEW, None)

                self.windows_view_stack[-1].update()

        print('Exiting dashcam GUI')
        self.running = False
        # pop and close any views left
        while not self.windows_view_stack:
            self.windows_view_stack.pop().close_view()

    def main_view_view_setup(self):
        return GUIMainView()

    def view_camera_view_setup(self):
        items = []
        if 'get_cameras' in registered_callbacks:
            items = registered_callbacks['get_cameras']()
        return GUICameraListView(items)

    def view_files_view_setup(self):
        updated_items = []
        if 'get_recordings_paths' in registered_callbacks:
            paths = registered_callbacks['get_recordings_paths']()
            for path in paths:
                filename = os.path.basename(path)
                updated_items.append((path, filename))
        if not updated_items:
            self.recordings_files = [('/', 'no files')]
        else:
            self.recordings_files = updated_items
        return GUIRecordingsListView(self.recordings_files)

    def lane_warning_view_setup(self):
        return GUILaneWarningView()

    def settings_view_setup(self):
        return GUISettingsView()

    def video_player_view_setup(self, video_player_packet):
        return GUIVideoPlayer(video_path=video_player_packet.video_path, stream=video_player_packet.stream)
