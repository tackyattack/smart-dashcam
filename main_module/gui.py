import sys
import threading
import tkinter as tk
import pprint
import Queue


if sys.version_info[0] >= 3:
    import PySimpleGUI as sg
else:
    import PySimpleGUI27 as sg

registered_callbacks = {
'calibrate': None,
'exit_gui': None
}

#sg.ChangeLookAndFeel('Dark')
#sg.SetOptions(element_padding=(1,1))

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

class GUIView(object):

    def __init__(self, view_stack):
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
        view_stack.append(self.view_window)

class GUIListView(GUIView):
    def __init__(self, view_stack, list_items):
        super(GUIListView, self).__init__(view_stack)

        bf = ('Helvetica', '15', 'bold')
        width=18
        height=1
        tk.Button(self.view_frame, text='Back',
                 bg='red', activebackground='red',activeforeground='white', fg='white',
                 font=bf,
                 width=width, height=height, command=self.exit_callback).pack()

        listbox = tk.Listbox(self.view_frame, width=19, font=("Helvetica", 30))

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
    def __init__(self, view_stack, list_items):
        super(GUICameraListView, self).__init__(view_stack, list_items)
class GUIRecordingsListView(GUIListView):
    def __init__(self, view_stack, list_items):
        super(GUIRecordingsListView, self).__init__(view_stack, list_items)

class GUIMainView(GUIView):
    def __init__(self, view_stack):
        super(GUIMainView, self).__init__(view_stack)
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
        put_window_command(WINDOW_COMMAND_PUSH_VIEW, GUIRecordingsListView)

    def view_camera_callback(self):
        put_window_command(WINDOW_COMMAND_PUSH_VIEW, GUICameraListView)

    def settings_callback(self):
        print('settings')

class DashcamGUI:

    def __init__(self, exit_callback):

        self.add_event_callback('exit_gui', exit_callback)
        self.windows_view_stack = []

        self.window = None
        self.running = True

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
        GUIMainView(self.windows_view_stack)

    def event_thread(self):
        self.setup_GUI()

        while self.running:
            # read only top of view stack since that's the only visible one
            event, values = self.windows_view_stack[-1].Read(timeout=100)
            # window closed (through exit icon -- so we probably won't need this)
            if event is None:
                put_window_command(WINDOW_COMMAND_POP_VIEW, None)

            while not window_command_queue.empty():
                window_command = window_command_queue.get(block=True)
                if window_command[0] == WINDOW_COMMAND_POP_VIEW:
                    self.windows_view_stack.pop().Close()
                if window_command[0] == WINDOW_COMMAND_PUSH_VIEW:
                    if window_command[1] == GUIRecordingsListView:
                        self.view_files_callback()
                    if window_command[1] == GUICameraListView:
                        self.view_camera_callback()

        print('Exiting dashcam GUI')
        self.running = False
        # pop and close any views left
        while not self.windows_view_stack:
            self.windows_view_stack.pop().Close()

    def view_camera_callback(self):
        items = []
        for i in range(20):
            items.append("camera " + str(i))
        GUICameraListView(self.windows_view_stack, items)

    def view_files_callback(self):
        items = []
        for i in range(20):
            items.append("video " + str(i))
        GUIRecordingsListView(self.windows_view_stack, items)
