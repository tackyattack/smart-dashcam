import sys
if sys.version_info[0] >= 3:
    import PySimpleGUI as sg
else:
    import PySimpleGUI27 as sg


sg.ChangeLookAndFeel('Dark')
sg.SetOptions(element_padding=(1,1))

def start_gui():

layout = [


      [sg.Button('Rear Camera', button_color=('white', 'red'), size=(20, 5), key='Rear Camera'),
      sg.Button('Other', button_color=('white', 'blue'), size=(20, 5), key='Other')],
      [sg.FilesBrowse('Browse Recordings', button_color=('white', 'green'), size=(20, 5), key='Browse'),
      sg.Button('Exit', button_color=('black', 'yellow'), size=(20, 5), key='Exit')],


    ]

window = sg.Window('Smart Dashcam GUI', layout, no_titlebar=True, location=(0,0), size=(480,320), keep_on_top=True)

while True:  # Event Loop
        event, values = window.Read()
        if event in (None, 'Exit'):
            break
        if event == 'Rear Camera':
            func('Rear Camera')
        elif event == 'Other':
            func('Other')
window.Close()