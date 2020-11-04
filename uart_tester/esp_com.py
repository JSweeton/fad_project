import serial
import keyboard
import typing
from sys import platform
import time

key_input = 0

def set_key_input(KeyEvent: keyboard.KeyboardEvent):
    global key_input

    key_input = KeyEvent.name 


def do_every(period,f,*args):
    def g_tick():
        t = time.time()
        while True:
            t += period
            yield max(t - time.time(),0)
    g = g_tick()
    while True:
        time.sleep(next(g))
        f(*args)

def every_1_ms():
    print(time.time())

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

def get_com_port():
    com_port = ""

    if platform == "linux" or platform == "linux2":
        com_port = "/dev/ttyUSB0"
        print(f'Running linux system, com port is {com_port}') 
    elif platform == "darwin":
        com_port = '' #not sure
        print(f'Running MAC, com port is {com_port}')
    elif platform == "win32":
        com_port = "COM5"
        print(f'Running windows system, com port is {com_port}')

    return com_port

def main():

    keyboard.hook(set_key_input)

    try:
        esp_com = serial.Serial(get_com_port(), baudrate=115200)
        print_input(esp_com)
    except serial.SerialException:
        print("Serial Opening Failed")

# main()


do_every(.001,every_1_ms)