#!/usr/bin/env python
#
# Tool to send ESP32 serial data via FadSerialPackets
# Once this is running and connected, use "w" to write given data
# to ESP32. It will parse this data and return a transformed set of
# data packets, which will be displayed using the dsp_tools library
#
# Based off of Espressif's ESP Monitor Tool, but greatly reduced in complexity
# Originally copyrighted by Espressif:
# Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD

import os
try:
    import queue
except ImportError:
    import Queue as queue
import time
import sys
import serial
import serial.tools.miniterm as miniterm
import threading
import tools.dsp_tools as dt
from tools import com_tools
from tools.packet_handler import FadSerialPacketHandler

from tools.consts import CMD, TAGS, PACKETS as P, CTRL, SerialStopException

talking = dt.get_talking(3)
DATA_G = dt.to_discrete(4096 * talking + 2048)

ALGORITHMS_G = [P.ALGO_TEST, P.ALGO_WHITE_V1_0, P.ALGO_DELAY]

class ConsoleParser(object):

    def __init__(self, eol="CRLF"):
        self.translate_eol = {
            "CRLF": lambda c: c.replace("\n", "\r\n"),
            "CR": lambda c: c.replace("\n", "\r"),
            "LF": lambda c: c.replace("\r", "\n"),
        }[eol]
        self.exit_key = CTRL.CTRL_RBRACKET
        self.send_data_key = CTRL.SEND_DATA_KEY 
        self.save_data_key = CTRL.SAVE_DATA_KEY

    def parse(self, key):
        ret = None
        if key == self.exit_key:
            ret = (TAGS.TAG_CMD, CMD.CMD_STOP)
        elif key == self.send_data_key:
            ret = (TAGS.TAG_CMD, CMD.CMD_SEND_DATA)
        elif key == self.save_data_key:
            ret = (TAGS.TAG_CMD, CMD.CMD_SAVE_DATA)
        else:
            key = self.translate_eol(key)
            ret = (TAGS.TAG_KEY, key)
            print("Use 'w' key to send data")
        return ret


class FadMonitor(object):
    '''
    Monitor class for ESP32, for use with FAD project. Has ability to send and receive
    chunks of data for the ESP32 to process, and for this program to display. Send data should
    be formatted as integer array
    '''

    def __init__(self, serial_instance, eol="CRLF", send_data=b'None', test_algorithms=[P.ALGO_TEST], send_data_byte_size = 2):
        super(FadMonitor, self).__init__()
        self.event_queue = queue.Queue()
        self.cmd_queue = queue.Queue()
        self.console = miniterm.Console() 

        self.serial = serial_instance
        self.console_parser = ConsoleParser(eol)
        self.console_reader = com_tools.ConsoleReader(self.console, self.event_queue, self.cmd_queue, self.console_parser)
        self.serial_reader = com_tools.SerialReader(self.serial, self.event_queue)
        self.packet_handler = FadSerialPacketHandler(send_data, test_algorithms, send_data_byte_size)

        self._last_line_part = b''
        self._invoke_processing_last_line_timer = None
        self._receiving_packet = False
        self._packet_buffer = b''
    
    def _invoke_processing_last_line(self):
        self.event_queue.put((TAGS.TAG_SERIAL_FLUSH, b''), False)

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
                if event_tag == TAGS.TAG_CMD:
                    self.handle_commands(data)

                elif event_tag == TAGS.TAG_KEY:
                    pass

                elif event_tag == TAGS.TAG_SERIAL:
                    self.handle_serial_input(data)
                    if self._invoke_processing_last_line_timer is not None:
                        self._invoke_processing_last_line_timer.cancel()
                    self._invoke_processing_last_line_timer = threading.Timer(.03, self._invoke_processing_last_line)
                    self._invoke_processing_last_line_timer.start()

                elif event_tag == TAGS.TAG_SERIAL_FLUSH:
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
            if line[0:4] == P.DATA_HEADER[0:4]:
                if len(line) == 271:    # Checks to make sure there were no line breaks within packet
                    self.packet_handler.add_packet(line)
                    self._receiving_packet = False
                else:
                    self._receiving_packet = True
                    self._packet_buffer = line + b'\n'
            
            elif self._receiving_packet:  # This means a packet has been started but cut off by line breaks.
                if (len(line) > 2 and line[-3:] == P.DATA_FOOTER[0:3]):
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
        packet = self.packet_handler.next_packet()
        print(self.packet_handler.message)
        while packet != None:
            self.serial.write(packet)
            packet = self.packet_handler.next_packet()

        print(self.packet_handler.message)
    
    def handle_commands(self, cmd):
        if (cmd == CMD.CMD_SEND_DATA):
            # Send data with serial write
            self.send_data()

        elif (cmd == CMD.CMD_SAVE_DATA):
            self.packet_handler.save_data()

        elif (cmd == CMD.CMD_STOP):
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
    baud = baud * 4
    serial_instance = serial.serial_for_url(port, baud,
                                            do_not_open=True)
    serial_instance.dtr = False
    serial_instance.rts = False

    offset = 30000
    large_song = dt.get_song(2)[0 + offset:51200 + offset]

    monitor = FadMonitor(serial_instance, send_data=DATA_G, test_algorithms = ALGORITHMS_G, send_data_byte_size=2)
    sys.stderr.write('--- fad_monitor on {p.name} {p.baudrate} ---'.format(
        p=serial_instance))

    monitor.main_loop()



if __name__ == "__main__":
    main()
