import sys
import threading
import tkinter as tk
import pprint
import Queue

if sys.version_info[0] >= 3:
    import PySimpleGUI as sg
else:
    import PySimpleGUI27 as sg


sg.ChangeLookAndFeel('Dark')
sg.SetOptions(element_padding=(1,1))

class DashcamGUI:

    FULL_SCREEN_WIDTH = 482
    FULL_SCREEN_HEIGHT = 322
    TOP_BAR_OFFSET = -30
    LEFT_OFFSET = -2

    WINDOW_COMMAND_CLOSE = 1

    def __init__(self, exit_callback):

        self.exit_callback = exit_callback
        self.event_callbacks = dict()
        self.windows = []
        self.window_command_queue = Queue.Queue()

        self.layout = [
                  [sg.Button('View Camera', button_color=('white', 'red'), size=(25, 8), key='View Camera'),
                  sg.Button('Calibrate Lane Detection', button_color=('white', 'blue'), size=(25, 8), key='Calibrate')],
                  [sg.Button('Browse Recordings', button_color=('white', 'green'), size=(25, 8), key='Browse'),
                  sg.Button('Exit', button_color=('black', 'yellow'), size=(25, 8), key='Exit')],
                ]

        self.window = None
        self.running = True

    def start(self):
        try:
            self.event_thread()
        except KeyboardInterrupt:
            self.terminate()


    def terminate(self):
        self.running = False
        self.exit_callback()

    def add_event_callback(self, event_name, callback_function):
        self.event_callbacks[event_name] = callback_function

    def get_window_by_title(self, name):
        i = 0
        for window_obj in self.windows:
            if(window_obj[1] in name):
                return i
            i = i + 1
        return None

    def put_window_command(self, window_name, command):
        self.window_command_queue.put((window_name, command))


    def files_list_press(self, event):
        w = event.widget
        index = int(w.curselection()[0])
        value = w.get(index)
        print 'You selected item %d: "%s"' % (index, value)
        self.put_window_command('files selector', self.WINDOW_COMMAND_CLOSE)

    def camera_list_press(self, event):
        w = event.widget
        index = int(w.curselection()[0])
        value = w.get(index)
        print 'You selected item %d: "%s"' % (index, value)
        self.put_window_command('camera selector', self.WINDOW_COMMAND_CLOSE)

    def create_list_view(self, name, items, callback):
        layout2 = [
                  [sg.Button('Exit', button_color=('white', 'red'), size=(320, 1), key='Exit List')],
                ]

        window2 = sg.Window(name, layout2, no_titlebar=False, location=(self.LEFT_OFFSET,self.TOP_BAR_OFFSET),
                           size=(self.FULL_SCREEN_WIDTH,self.FULL_SCREEN_HEIGHT),
                            grab_anywhere=False, keep_on_top=True, background_color='black')
        window2.Finalize()
        tk_window2 = window2.TKroot
        tk_window2.config(cursor="none")
        self.windows.append((window2, name))

        listbox = tk.Listbox(tk_window2, width=19, font=("Helvetica", 30))

        for list_item in items:
            listbox.insert(tk.END, list_item)
        listbox.pack(side="left", fill="both")

        scrollbar = tk.Scrollbar(tk_window2, width=30,orient="vertical")
        scrollbar.config(command=listbox.yview)
        scrollbar.pack(side="left", fill="both")

        listbox.config(yscrollcommand=scrollbar.set)

        listbox.bind('<<ListboxSelect>>', callback)

    def setup_GUI(self):
        # note: window stuff needs to be in same thread
        window = sg.Window('Smart Dashcam GUI', self.layout, no_titlebar=False, location=(self.LEFT_OFFSET, self.TOP_BAR_OFFSET),
        size=(self.FULL_SCREEN_WIDTH,self.FULL_SCREEN_HEIGHT), grab_anywhere=False, background_color='black')
        window.Finalize()
        tk_window = window.TKroot
        tk_window.config(cursor="none")
        self.windows.append((window, 'main gui'))

    def event_thread(self):
        self.setup_GUI()

        while self.running:  # Event Loop
                for window_obj in self.windows:
                    event, values = window_obj[0].Read(timeout=100)
                    # window closed (through exit icon -- so we probably won't need this)
                    if event is None:
                        break
                    if event == 'View Camera':
                        self.view_camera_callback()
                    elif event == 'Exit':
                        self.terminate()
                    elif event == 'Browse':
                        self.view_files_callback()
                    elif event == 'Exit List':
                        window_obj[0].Close()
                        self.windows.remove(window_obj)

                # check for registered callbacks
                if event in self.event_callbacks:
                    self.event_callbacks[event]()

                while not self.window_command_queue.empty():
                    window_command = self.window_command_queue.get(block=True)
                    if window_command[1] == self.WINDOW_COMMAND_CLOSE:
                        window_index = self.get_window_by_title(window_command[0])
                        self.windows[window_index][0].Close()
                        del self.windows[window_index]

        print('Exiting dashcam GUI')
        self.running = False
        for window_obj in self.windows:
            window_obj[0].Close()
        self.windows = []

    def view_camera_callback(self):
        items = []
        for i in range(20):
            items.append("" + str(i))
        self.create_list_view('camera selector', items, self.camera_list_press)
        print('show cameras')
    def view_files_callback(self):
        items = []
        for i in range(20):
            items.append("" + str(i))
        self.create_list_view('files selector', items, self.camera_list_press)
        print('show cameras')
