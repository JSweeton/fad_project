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
import dsp_tools as dt

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

PACKET_DATA_SIZE = 256
DATA_HEADER = codecs.encode("DATA\x00")
DATA_FOOTER = codecs.encode("END\n")
PACKET_SIZE = 256

# Algorithm Choices (enum-like)
ALGO_WHITE_V1_0 = b'ALGO_WHITE_V1_0' 
ALGO_TEST = b'ALGO_TEST'

# Data Types (bytes-like)
ONE_BYTE_UNSIGNED = b'U1\0'
TWO_BYTE_UNSIGNED = b'U2\0'
ALGORITHM_SELECT = b'AS\0'


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
            import fcntl
            import termios
            fcntl.ioctl(self.console.fd, termios.TIOCSTI, b'\0')

class ConsoleParser(object):

    def __init__(self, eol="CRLF"):
        self.translate_eol = {
            "CRLF": lambda c: c.replace("\n", "\r\n"),
            "CR": lambda c: c.replace("\n", "\r"),
            "LF": lambda c: c.replace("\r", "\n"),
        }[eol]
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

class FadSerialPacketHandler():

    class FadPacket():
        '''This class defines the components of a serial data packet
        which holds simulated audio data to be sent between the ESP32
        and the python serial program. The data consists of a header with
        a Name and a type e.g. b'DATA/0U1/0' (but back slashes) denotes a fad packet
        with a data type of 'U1', or a packet of char values.'''
        def __init__(self, data):
            err = self.bad_data(data)
            if err:
                raise TypeError(err)
            self.data = data[8:PACKET_DATA_SIZE  + 8]
            self.type = data[5:7]
            self.packets_incoming = int.from_bytes(data[-7:-5], byteorder='little')
            self.error_id = int.from_bytes(data[-5:-3], byteorder='little')

        def bad_data(self, data):
            if data[0:4] != DATA_HEADER[0:4]:
                return "Invalid Header" 

            elif data[-3:] != DATA_FOOTER[0:3]:
                print()
                return "Invalid footer" 

        def __str__(self):
            return f'Data: {self.data.hex()}'

    PACKET_DUMP_SIZE = 10   # Sets the number of packets sent at a time
    def __init__(self, send_data, algorithms, byte_size):
        self.receive_buffer = []

        self.original_data = send_data
        self.in_data = self.partition_data(send_data, byte_size)
        self.in_data_pos = 0 
        self.in_data_len = len(self.in_data)
        self.missed_packet_queue = queue.Queue()

        self.algorithms = algorithms
        self.algorithms_pos = len(algorithms)

        self.first_packet = False   # keeps track of whether a first packet has been received
        self.last_packet = 0        # keeps track of number of last packet
        self.message = ''
        self.finished_algorithm = False
        self.ignore_dump = False
        
    def partition_data(self, data_to_send, int_size_in_bytes):
        total_length = len(data_to_send)
        data_array = []
        array_vals_per_packet = PACKET_SIZE // int_size_in_bytes
        num_packets = total_length * int_size_in_bytes // PACKET_SIZE
        for i in range(num_packets):
            data_as_bytes = b''
            for j in range(array_vals_per_packet): # each int pos is equivalent to size
                data_pos = i * array_vals_per_packet + j
                data_as_bytes += data_to_send[data_pos].to_bytes(int_size_in_bytes, byteorder='little')

            data_array.append(data_as_bytes)
        
        return data_array

    def add_packet(self, packet_data):

        try:
            packet = self.FadPacket(packet_data)
            self.receive_buffer.append(packet)

            # Check for missed packets
            packet_jump = self.last_packet - packet.packets_incoming
            if (self.first_packet and packet_jump != 1):
                print(f"Missed {packet_jump - 1} packets after " + str(packet.packets_incoming + packet_jump))

            self.last_packet = packet.packets_incoming

            if packet.packets_incoming == 0:
                self.display_received_packets()
                self.clear_data()

        except TypeError as err:
            # for now, just print
            print(repr(err))
        

        if not self.first_packet:
            self.first_packet = True
        
    def next_packet(self):
        '''Returns the next packet to be sent, based on last packet sent. Returns None if none left'''

        packets_left = self.in_data_len - self.in_data_pos - 1

        if (packets_left < 0):
            self.message = f"Finished current packet set. Algorithms remaining: {self.algorithms_pos + 1}"
            self.finished_algorithm = True
            return None

        if ((packets_left + 1) % self.PACKET_DUMP_SIZE == 0 and not self.ignore_dump):
            self.message = f"Packet dump sent. Continue, packets left: {packets_left}"
            self.ignore_dump = True
            return None

        self.ignore_dump = False 

        try:
            missed_packet = self.missed_packet_queue.get_nowait()
        except queue.Empty:
            missed_packet = 0

        packet = self._create_generic_packet(self.in_data[self.in_data_pos], ONE_BYTE_UNSIGNED, packets_left, missed_packet)
        self.in_data_pos += 1

        return packet


    
    def initial_packet(self):
        '''Returns an initiation packet based on next algorithm choice'''
        self.algorithms_pos -= 1

        if (self.finished_algorithm == False):
            return None

        if (self.algorithms_pos < 0):
            self.message = "Out of algorithms."
            self.algorithms_pos = len(self.algorithms)
            return None

        algo = self.algorithms[self.algorithms_pos] 
        data = algo + b'\0' * (PACKET_DATA_SIZE - len(algo))
        self.finished_algorithm = False
        return self._create_generic_packet(data, ALGORITHM_SELECT, len(self.in_data), 0)

    def _create_generic_packet(self, data, data_type, packets_left, packets_missed):
            packet = DATA_HEADER
            packet += data_type
            packet += data
            packet += packets_left.to_bytes(2, byteorder='little')
            packet += packets_missed.to_bytes(2, byteorder='little')
            packet += DATA_FOOTER
            return packet

    def clear_data(self):
        self.receive_buffer = []
        self.first_packet = False
        self.last_packet = 0
        
    def display_received_packets(self):
        # Stitch together data within packets
        all_data = []   # Holds stitched data from each received packet
        for packet in self.receive_buffer:
            packet_data = list(packet.data)
            for data in packet_data:
                all_data.append(data * 16)

        dt.play(dt.center(all_data))
        dt.plot([all_data, self.original_data], labels=["Packet Data", "Input Data"])
        dt.show()

class FadMonitor(object):
    '''
    Monitor class for ESP32, for use with FAD project. Has ability to send and receive
    chunks of data for the ESP32 to process, and for this program to display. Send data should
    be formatted as integer array
    '''

    def __init__(self, serial_instance, eol="CRLF", send_data=b'None', test_algorithms=[ALGO_TEST], send_data_byte_size = 2):
        super(FadMonitor, self).__init__()
        self.event_queue = queue.Queue()
        self.cmd_queue = queue.Queue()
        self.console = miniterm.Console() 

        self.serial = serial_instance
        self.console_parser = ConsoleParser(eol)
        self.console_reader = ConsoleReader(self.console, self.event_queue, self.cmd_queue, self.console_parser)
        self.serial_reader = SerialReader(self.serial, self.event_queue)
        self.packet_handler = FadSerialPacketHandler(send_data, test_algorithms, send_data_byte_size)

        self._last_line_part = b''
        self._invoke_processing_last_line_timer = None
        self._receiving_packet = False
        self._packet_buffer = b''
    
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
            if line[0:4] == DATA_HEADER[0:4]:
                if len(line) == 271:    # Checks to make sure there were no line breaks within packet
                    self.packet_handler.add_packet(line)
                    self._receiving_packet = False
                else:
                    self._receiving_packet = True
                    self._packet_buffer = line + b'\n'
            
            elif self._receiving_packet:  # This means a packet has been started but cut off by line breaks.
                if (len(line) > 2 and line[-3:] == DATA_FOOTER[0:3]):
                    self.packet_handler.add_packet(self._packet_buffer + line)
                    self._receiving_packet = False
                else:
                    self._packet_buffer += line + b'\n'

            else: 
                self._print(line + b'\n')

        if finalize_line:
            self._print(self._last_line_part + b'\n')
            self._last_line_part = b''
    
    def send_data(self):
        print("Beginning packet transmission...")
        # packet = self.packet_handler.initial_packet() 
        packet = b''
        while packet != None:
            self.serial.write(packet)
            packet = self.packet_handler.next_packet()

        print(self.packet_handler.message)

    def handle_commands(self, cmd):
        if (cmd == CMD_SEND_DATA):
            # Send data with serial write
            self.send_data()

        elif (cmd == CMD_STOP):
            self.serial_reader.stop()
            self.console_reader.stop()

    def _print(self, string):
        self.console.write_bytes(string)


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

    large_song = dt.get_song(2)[64640:95360]
    small_song = dt.get_song(2)[80000:88192]
    one_packet = small_song[0:256]
    my_packet = dt.discrete_square_wave(20, 256)
    for i in range(255):
        my_packet[i] += 30 

    my_packet[5] = 0
    my_data = dt.to_discrete(my_packet) # Square wave with amplitude 512 and width 20

    monitor = FadMonitor(serial_instance, send_data=my_data, send_data_byte_size=1)
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
