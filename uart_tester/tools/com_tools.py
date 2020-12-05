# com_tools.py
# Author: Corey Bean
#
# Includes classes needed for main function such as the StobbableThread item, 
# a tool that gathers console input, and a tool that gathers serial data.
#

import serial
import sys
import threading
import time
import os

from tools.consts import TAGS, CMD

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
                        if tag == TAGS.TAG_CMD and cmd != CMD.CMD_STOP:
                            self.cmd_queue.put(ret)
                        else:
                            self.event_queue.put(ret)

        finally:
            self.console.cleanup()

    def _cancel(self): # See monitor.py file in idf tools folder for explanation
        if os.name == 'posix':
            import fcntl # pylint: disable=import-error
            import termios # pylint: disable=import-error

            fcntl.ioctl(self.console.fd, termios.TIOCSTI, b'\0')

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
                    self.event_queue.put((TAGS.TAG_SERIAL, data), False)
        finally:
            self.serial.close()

    def _cancel(self):
        if hasattr(self.serial, 'cancel_read'):
            try:
                self.serial.cancel_read()
            except Exception:
                pass
