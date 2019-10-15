import PySimpleGUI27 as sg

def start_gui():
    sg.ChangeLookAndFeel('Dark')
    sg.SetOptions(element_padding=(4, 4))

    layout = [

        [sg.Button('Rear Camera', button_color=('white', 'red'), size=(20, 5), key='Rear Camera'),
         sg.Button('Function1', button_color=('white', 'blue'), size=(20, 5), key='Function1'),
         sg.Button('Function2', button_color=('white', 'green'), size=(20, 5), key='Function2'),
         sg.Button('Function3', button_color=('white', 'yellow'), size=(20, 5), key='Function3')]
        ]

    window = sg.Window('Smart Dashcam GUI', layout, no_titlebar=True, location=(0,0), size=(480,320), keep_on_top=True)

    while True:             # Event Loop
        event, values = window.Read()
        if event in (None, 'Exit'):
            break
        if event == 'Rear Camera':
            func('Rear Camera')
        elif event == 'Function 1':
            func('Function 1')
        elif event == 'Function 2':
            func('Function 2')
        elif event == 'Function 3':
            func('Function 3')
    window.Close()

    event, values = window.Read()
    text_input = values[0]
