import serial
import keyboard
import typing
from sys import platform

key_input = 0

def set_key_input(KeyEvent: keyboard.KeyboardEvent):
    global key_input

    key_input = KeyEvent.name 

    # debug
    # print(key_input)

def print_input(com_instance: serial.Serial):

    print("Successfully opened com")

    while True:
        global key_input
        input_bytes = com_instance.read()
        print(input_bytes.decode(), end='')
        # for now, continually output a 'y'
        output_bytes = bytes('yyyyy\n', 'utf-8')
        com_instance.write(output_bytes)
       # if key_input != 0:
        #     output_string = key_input
        #     key_input = 0
        #     output_bytes = bytes(output_string, 'utf-8')  
        #     com_instance.write(output_bytes) 

def main():
    if platform == "linux" or platform == "linux2":
        com_port = "/dev/ttyUSB0"
        print(f'Running linux system, com port is {com_port}')
    elif platform == "darwin":
        com_port = '' #not sure
        print(f'Running MAC, com port is {com_port}')
    elif platform == "win32":
        com_port = "COM5"
        print(f'Running windows system, com port is {com_port}')

    # keyboard.on_press_key("q", lambda _: set_key_input('q'))
    # keyboard.on_press_key("w", lambda _: set_key_input('w'))
    # keyboard.on_press_key("e", lambda _: set_key_input('e'))
    # keyboard.on_press_key("r", lambda _: set_key_input('r'))

    keyboard.hook(set_key_input)

    try:
        esp_com = serial.Serial(com_port, baudrate=115200)
        print_input(esp_com)
    except serial.SerialException:
        print("COM port not opened")

    while(True):
       sudo = True

main()

