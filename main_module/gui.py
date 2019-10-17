import sys
import threading

if sys.version_info[0] >= 3:
    import PySimpleGUI as sg
else:
    import PySimpleGUI27 as sg


sg.ChangeLookAndFeel('Dark')
sg.SetOptions(element_padding=(1,1))

class DashcamGUI:
    def __init__(self, exit_callback):

        self.exit_callback = exit_callback
        self.event_callbacks = dict()

        self.layout = [
                  [sg.Button('Rear Camera', button_color=('white', 'red'), size=(20, 5), key='Rear Camera'),
                  sg.Button('Other', button_color=('white', 'blue'), size=(20, 5), key='Other')],
                  [sg.FilesBrowse('Browse Recordings', button_color=('white', 'green'), size=(20, 5), key='Browse'),
                  sg.Button('Exit', button_color=('black', 'yellow'), size=(20, 5), key='Exit')],
                ]

        self.window = None
        self.running = True

        self.event_thread = threading.Thread(target=self.event_thread)
        self.event_thread.start()

    def terminate(self):
        self.running = False
        self.exit_callback()

    def add_event_callback(self, event_name, callback_function):
        self.event_callbacks[event_name] = callback_function


    def event_thread(self):
        # note: window stuff needs to be in same thread
        self.window = sg.Window('Smart Dashcam GUI', self.layout, no_titlebar=False, location=(0,-35), size=(480,320), grab_anywhere=False)
        self.window.Finalize()
        #self.window.TKroot.lift()
        #self.window.TKroot.update()
        while self.running:  # Event Loop
                event, values = self.window.Read(timeout=100)
                # window closed (through exit icon -- so we probably won't need this)
                if event is None:
                    break
                if event == 'Rear Camera':
                    self.rear_camera_callback()
                elif event == 'Exit':
                    self.terminate()
                elif event == 'Other':
                    pass
                elif event == 'Browse Recordings':
                    pass

                # check for registered callbacks
                if event in self.event_callbacks:
                    self.event_callbacks[event]()

        print('Exiting dashcam GUI')
        self.running = False
        self.window.Close()

    def rear_camera_callback(self):
        print('rear camera button clicked')
