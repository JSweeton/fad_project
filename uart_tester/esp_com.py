#!/usr/bin/env python
#
# esp-idf serial output monitor tool. Does some helpful things:
# - Looks up hex addresses in ELF file with addr2line
# - Reset ESP32 via serial RTS line (Ctrl-T Ctrl-R)
# - Run flash build target to rebuild and flash entire project (Ctrl-T Ctrl-F)
# - Run app-flash build target to rebuild and flash app only (Ctrl-T Ctrl-A)
# - If gdbstub output is detected, gdb is automatically loaded
# - If core dump output is detected, it is converted to a human-readable report
#   by espcoredump.py.
#
# Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Contains elements taken from miniterm "Very simple serial terminal" which
# is part of pySerial. https://github.com/pyserial/pyserial
# (C)2002-2015 Chris Liechti <cliechti@gmx.net>
#
# Originally released under BSD-3-Clause license.
#
from __future__ import print_function, division
from __future__ import unicode_literals
from builtins import chr
from builtins import object
from builtins import bytes
import subprocess
import argparse
import codecs
import datetime
import re
import os
try:
    import queue
except ImportError:
    import Queue as queue
import shlex
import time
import sys
import serial
import serial.tools.list_ports
import serial.tools.miniterm as miniterm
import threading
import ctypes
import types
from distutils.version import StrictVersion
from io import open
import textwrap
import tempfile
import json
import dsp_tools

try:
    import websocket
except ImportError:
    # This is needed for IDE integration only.
    pass

key_description = miniterm.key_description

# Control-key characters
CTRL_A = '\x01'
CTRL_B = '\x02'
CTRL_F = '\x06'
CTRL_H = '\x08'
CTRL_R = '\x12'
CTRL_T = '\x14'
CTRL_Y = '\x19'
CTRL_P = '\x10'
CTRL_X = '\x18'
CTRL_L = '\x0c'
CTRL_RBRACKET = '\x1d'  # Ctrl+]
SEND_DATA_KEY = 'w'

# Command parsed from console inputs
CMD_STOP = 1
CMD_RESET = 2
CMD_MAKE = 3
CMD_APP_FLASH = 4
CMD_OUTPUT_TOGGLE = 5
CMD_TOGGLE_LOGGING = 6
CMD_ENTER_BOOT = 7
CMD_SEND_DATA = 8

__version__ = "1.1"

# Tags for tuples in queues
TAG_KEY = 0
TAG_SERIAL = 1
TAG_SERIAL_FLUSH = 2
TAG_CMD = 3

PACKET_SIZE = 256
DATA_HEADER = codecs.encode("DATA\x00")
DATA_FOOTER = codecs.encode("ENDSIG\x00\n")
PACKET_SIZE = 256


class StoppableThread(object):
    """
    Provide a Thread-like class which can be 'cancelled' via a subclass-provided
    cancellation method.

    Can be started and stopped multiple times.

    Isn't an instance of type Thread because Python Thread objects can only be run once
    """
    def __init__(self):
        self._thread = None

    @property
    def alive(self):
        """
        Is 'alive' whenever the internal thread object exists
        """
        return self._thread is not None

    def start(self):
        if self._thread is None:
            self._thread = threading.Thread(target=self._run_outer)
            self._thread.start()

    def _cancel(self):
        pass  # override to provide cancellation functionality

    def run(self):
        pass  # override for the main thread behaviour

    def _run_outer(self):
        try:
            self.run()
        finally:
            self._thread = None

    def stop(self):
        if self._thread is not None:
            old_thread = self._thread
            self._thread = None
            self._cancel()
            old_thread.join()

class ConsoleReader(StoppableThread):
    """ Read input keys from the console and push them to the queue,
    until stopped.
    """
    def __init__(self, console, event_queue, cmd_queue, parser):
        super(ConsoleReader, self).__init__()
        self.console = console
        self.event_queue = event_queue
        self.cmd_queue = cmd_queue
        self.parser = parser

    def run(self):
        self.console.setup()
        try:
            while self.alive:
                try:
                    if os.name == 'nt':
                        # Windows kludge: because the console.cancel() method doesn't
                        # seem to work to unblock getkey() on the Windows implementation.
                        #
                        # So we only call getkey() if we know there's a key waiting for us.
                        import msvcrt
                        while not msvcrt.kbhit() and self.alive:
                            time.sleep(0.1)
                        if not self.alive:
                            break
                    c = self.console.getkey()
                except KeyboardInterrupt:
                    c = '\x03'
                if c is not None:
                    ret = self.parser.parse(c)
                    if ret is not None:
                        (tag, cmd) = ret
                        # stop command should be executed last
                        if tag == TAG_CMD and cmd != CMD_STOP:
                            self.cmd_queue.put(ret)
                        else:
                            self.event_queue.put(ret)

        finally:
            self.console.cleanup()

    def _cancel(self): # See monitor.py file in idf tools folder for explanation
        if os.name == 'posix':
            # import fcntl
            # import termios
            # fcntl.ioctl(self.console.fd, termios.TIOCSTI, b'\0')
            pass

class ConsoleParser(object):

    def __init__(self, eol="CRLF"):
        self.translate_eol = {
            "CRLF": lambda c: c.replace("\n", "\r\n"),
            "CR": lambda c: c.replace("\n", "\r"),
            "LF": lambda c: c.replace("\r", "\n"),
        }[eol]
        self.menu_key = CTRL_T
        self.exit_key = CTRL_RBRACKET
        self.send_data_key = SEND_DATA_KEY 

    def parse(self, key):
        ret = None
        if key == self.exit_key:
            ret = (TAG_CMD, CMD_STOP)
        elif key == self.send_data_key:
            ret = (TAG_CMD, CMD_SEND_DATA)
        else:
            key = self.translate_eol(key)
            ret = (TAG_KEY, key)
        return ret

class SerialReader(StoppableThread):
    """ Read serial data from the serial port and push to the
    event queue, until stopped.
    """
    def __init__(self, serial, event_queue):
        super(SerialReader, self).__init__()
        self.baud = serial.baudrate
        self.serial = serial
        self.event_queue = event_queue
        if not hasattr(self.serial, 'cancel_read'):
            # enable timeout for checking alive flag,
            # if cancel_read not available
            self.serial.timeout = 0.25

    def run(self):
        if not self.serial.is_open:
            self.serial.baudrate = self.baud
            self.serial.rts = True  # Force an RTS reset on open
            self.serial.open()
            self.serial.rts = False
            self.serial.dtr = self.serial.dtr   # usbser.sys workaround
        try:
            while self.alive:
                try:
                    data = self.serial.read(self.serial.in_waiting or 1)
                except (serial.serialutil.SerialException, IOError):
                    data = b''
                    # self.serial.open() was successful before, therefore, this is an issue related to
                    # the disapperence of the device
                    sys.stderr.write('Waiting for the device to reconnect')
                    self.serial.close()
                    while self.alive:  # so that exiting monitor works while waiting
                        try:
                            time.sleep(0.5)
                            self.serial.open()
                            break  # device connected
                        except serial.serialutil.SerialException:
                            sys.stderr.write('.')
                            sys.stderr.flush()
                    sys.stderr.write('\n')  # go to new line
                if len(data):
                    self.event_queue.put((TAG_SERIAL, data), False)
        finally:
            self.serial.close()

    def _cancel(self):
        if hasattr(self.serial, 'cancel_read'):
            try:
                self.serial.cancel_read()
            except Exception:
                pass

class SerialStopException(Exception):
    """
    This exception is used for stopping the IDF monitor in testing mode.
    """
    pass

class FadMonitor(object):
    '''
    Monitor class for ESP32, for use with FAD project. Has ability to send and receive
    chunks of data for the ESP32 to process, and for this program to display. Send data should
    be formatted as integer array
    '''

    def __init__(self, serial_instance, eol="CRLF", send_data=b'None', send_data_byte_size = 2):
        super(FadMonitor, self).__init__()
        self.event_queue = queue.Queue()
        self.cmd_queue = queue.Queue()
        self.console = miniterm.Console() 
        # if os.name == 'nt':
        #     sys.stderr = ANSIColorConverter(sys.stderr, decode_output=True)
        #     self.console.output = ANSIColorConverter(self.console.output)
        #     self.console.byte_output = ANSIColorConverter(self.console.byte_output)

        self.serial = serial_instance
        self.console_parser = ConsoleParser(eol)
        self.console_reader = ConsoleReader(self.console, self.event_queue, self.cmd_queue, self.console_parser)
        self.serial_reader = SerialReader(self.serial, self.event_queue)
        self.data_array = self.partition_data(send_data, send_data_byte_size)

        self.last_packet = []
        self._last_line_part = b''
        self._invoke_processing_last_line_timer = None
        self._receiving_packet = False

    def partition_data(self, data_to_send, int_size_in_bytes):
        total_length = len(data_to_send)
        data_array = []
        array_vals_per_packet = PACKET_SIZE // int_size_in_bytes
        num_packets = total_length * int_size_in_bytes // PACKET_SIZE
        for i in range(num_packets):
            data_as_bytes = b''
            for j in range(array_vals_per_packet): # each int pos is equivalent to size
                data_pos = i * array_vals_per_packet + j
                data_as_bytes += data_to_send[data_pos].to_bytes(2, byteorder='big')

            data_array.append(data_as_bytes)
        
        return data_array

    def _invoke_processing_last_line(self):
        self.event_queue.put((TAG_SERIAL_FLUSH, b''), False)

    def main_loop(self):
        self.console_reader.start()
        self.serial_reader.start()
        try:
            while self.console_reader.alive and self.serial_reader.alive:
                try:
                    item = self.cmd_queue.get_nowait()
                except queue.Empty:
                    try:
                        item = self.event_queue.get(True, 0.03)
                    except queue.Empty:
                        continue

                (event_tag, data) = item
                if event_tag == TAG_CMD:
                    self.handle_commands(data)

                elif event_tag == TAG_KEY:
                    try:
                        self.serial.write(codecs.encode(data))
                    except serial.SerialException:
                        pass  # this shouldn't happen, but sometimes port has closed in serial thread
                    except UnicodeEncodeError:
                        pass  # this can happen if a non-ascii character was passed, ignoring

                elif event_tag == TAG_SERIAL:
                    self.handle_serial_input(data)
                    if self._invoke_processing_last_line_timer is not None:
                        self._invoke_processing_last_line_timer.cancel()
                    self._invoke_processing_last_line_timer = threading.Timer(.03, self._invoke_processing_last_line)
                    self._invoke_processing_last_line_timer.start()

                elif event_tag == TAG_SERIAL_FLUSH:
                    self.handle_serial_input(data, finalize_line=True)
                else:
                    raise RuntimeError("Bad event data %r" % ((event_tag,data),))
        except SerialStopException:
            sys.stderr.write("Stopping condition has been received\n")
        finally:
            try:
                self.console_reader.stop()
                self.serial_reader.stop()

            except Exception:
                pass
            sys.stderr.write("\n")

    def handle_serial_input(self, data, finalize_line=False):
        sp = data.split(b'\n')

        sp[0] = self._last_line_part + sp[0]
        # Places either empty byte or unfinished line of data in last line
        self._last_line_part = sp.pop()

        for line in sp:
            if line[0:4] == b'DATA':
                self.handle_packet(line)
            else: 
                self._print(line + b'\n')

        if finalize_line:
            self._print(self._last_line_part + b'\n')
            self._last_line_part = b''
    
    def send_data(self):
        n = len(self.data_array)
        print("Sending data...\n") 
        for i in range(n): 
            self.serial.write(DATA_HEADER)
            b = lambda x: x.to_bytes(1, byteorder='big') # used to convert 1 int to a byte
            self.serial.write(b(n - i - 1) + b(i))
            self.serial.write(self.data_array[n])
            self.serial.write(DATA_FOOTER)

    def handle_commands(self, cmd):
        if (cmd == CMD_SEND_DATA):
            # Send data with serial write
            self.send_data()

        elif (cmd == CMD_STOP):
            self.serial_reader.stop()
            self.console_reader.stop()

    def handle_packet(self, packet):
        try:
            print("PACKET RECEIVED")
            new_packet = FadSerialPacket(packet)

        except TypeError as err:
            print(repr(err))
            new_packet = None
            return
        
        if new_packet.packets_previous == 0 and new_packet.packets_incoming > 0:     # A new set of packets is incoming
            self.current_packet_set = [new_packet.data]


    def _print(self, string):
        self.console.write_bytes(string)

class FadSerialPacket():
    '''This class defines the components of a serial data packet
    which holds simulated audio data to be sent between the ESP32
    and the python serial program. The data consists of a header with
    a Name and a length e.g. b'DATA/0/x10/x10' (but back slashes) denotes a fad packet
    with a length of 0x1010, or a very large length.'''

    def __init__(self, data):
        err = self.bad_data(data)
        if err != None:
            raise TypeError(err)            

        self.data = data[8 : PACKET_SIZE + 8]   # Data as bytes
        self.packets_incoming = int(data[5])
        self.packets_previous = int(data[6])
        
    def bad_data(self, data):
        if data[0:4] != DATA_HEADER[0:4]:
            return "Invalid Header" 

        elif data[-7:] != DATA_FOOTER[0:7]:
            return "Invalid footer" 

    def __str__(self):
        return f'Data: {self.data.hex()}'

def main():
    def _get_default_serial_port():
        com_port = ""

        if sys.platform == "linux" or sys.platform == "linux2":
            com_port = "/dev/ttyUSB0"
            print(f'Running linux system, com port is {com_port}') 

        elif sys.platform == "win32":
            com_port = "COM5"
            print(f'Running windows system, com port is {com_port}')

        return com_port

    port = _get_default_serial_port()
    baud = 115200
    serial_instance = serial.serial_for_url(port, baud,
                                            do_not_open=True)
    serial_instance.dtr = False
    serial_instance.rts = False

    my_data = dsp_tools.to_discrete(512 * dsp_tools.square_wave(20, 256))

    monitor = FadMonitor(serial_instance, send_data=my_data)
    sys.stderr.write('--- fad_monitor on {p.name} {p.baudrate} ---'.format(
        p=serial_instance))

    # yellow_print('--- Quit: {} | Menu: {} | Help: {} followed by {} ---'.format(
    #     key_description(monitor.console_parser.exit_key),
    #     key_description(monitor.console_parser.menu_key),
    #     key_description(monitor.console_parser.menu_key),
    #     key_description(CTRL_H)))
    monitor.main_loop()



if __name__ == "__main__":
    main()
