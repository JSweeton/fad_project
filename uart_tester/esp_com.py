import serial
import keyboard
import typing
from sys import platform
import time
import dsp_tools

BUFFER_SIZE = 512 
class ESP_Com(serial.Serial):
    '''A serial UART for the ESP32. Based off of the PySerial Serial Class'''

    PAYLOAD_HEADER = bytearray("Begin Payload Transmission", 'utf-8')
    PAYLOAD_FOOTER = bytearray("End Payload Transmission", 'utf-8')

    def __init__(self, port: str, baudrate: int = 115200, default_data = 0):
        super().__init__(port, baudrate=baudrate, timeout=0.01) 
        self.output_buffer = []
        self.input_buffer = BUFFER_SIZE * [0]
        self.default_data = default_data
        self.monitor_is_live = False

    @property
    def default_data(self):
        return self._default_data 
        
    @default_data.setter
    def default_data(self, data):
        self._default_data = data

    @property
    def payload_data(self):
        return self._payload_data 
        
    @payload_data.setter
    def payload_data(self, data):
        self._payload_data = data
       
    def write(self, data):
        if not self.monitor_is_live:
            print("Written data before monitor began.")
            return

        while (len(self.output_buffer) > 0):
            pass

        self.output_buffer = data

    def read(self, size):
        input_bytes = super().read(size)
        return input_bytes

    def setup_key_input(self):
        keyboard.hook(self.key_handler)    

    def _parse_key(self, key_name: str):
        # TODO: Handle key input, do something
        if key_name == 'w':
            print(key_name)
            self.write(self.default_data)
        elif key_name =='e':
            self.end_monitor()
        elif key_name =='b':
            self.begin_payload_transmission()
        else:
            print(f'Unhandled key: {key_name}')

    def key_handler(self, KeyEvent: keyboard.KeyboardEvent):
        self._parse_key(KeyEvent.name)

    def begin_payload_transmission(self):
        print("Beginning transmission...")
        self.write(self.PAYLOAD_HEADER)
        self.write(self.payload_data)
        self.write(self.PAYLOAD_FOOTER)

    def end_monitor(self):
        self.break_monitor = True

    def begin_monitor(self):            
        print("Beginning monitor readings...")
        self.monitor_is_live = True
        self.break_monitor = False

        while (not self.break_monitor):
            read_bytes = self.read(1)

            try:
                read_string = read_bytes.decode()
            except UnicodeDecodeError:
                read_string = "\n[ERROR]: Invalid Input"
                        
            print(read_string, end="")

            #Check for new data to write
            if len(self.output_buffer) > 0:
                super().write(self.output_buffer)
                self.output_buffer = [] 

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

    serial_port = get_com_port()
    com = ESP_Com(serial_port, baudrate=115200)

    com.payload_data = bytearray([10, 43, 2, 1])
    com.setup_key_input()
    com.begin_monitor()
    
    com.begin_payload_transmission()

main()

# do_every(.001,every_1_ms)