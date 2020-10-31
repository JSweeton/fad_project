import serial
import keyboard


def set_key_input(char):
    global key_input

    key_input = char


def print_input(com_instance):

    global key_input

    while True:
        input_bytes = com_instance.read()
        print(input_bytes.decode(), end='')
        output_bytes = input_bytes
        
def main():

    keyboard.on_press_key("p", lambda _: set_key_input('p'))

    try:
        esp_com = serial.Serial("COM5", baudrate=115200)
        print_input(esp_com)
    except serial.SerialException:
        print("COM port not opened")


main()

