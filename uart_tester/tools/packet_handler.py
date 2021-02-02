import queue
from math import ceil
from tools import dsp_tools as dt
from tools.consts import PACKETS as P
import numpy as np
import datetime

class FadPacket():
    '''This class defines the components of a serial data packet
    which holds simulated audio data to be sent between the ESP32
    and the python serial program. The data consists of a header with
    a Name and a type e.g. b'DATA/0U1/0' (but back slashes) denotes a fad packet
    with a data type of 'U1', or a packet of char values.
    Raises TypeError if packet is bad.'''
    def __init__(self, data):
        err = self.bad_data(data)
        if err != 0:
            raise TypeError(err)

        self.data = data[8:P.PACKET_DATA_SIZE  + 8]
        self.type = data[5:7]
        self.packets_incoming = int.from_bytes(data[-7:-5], byteorder='little')
        self.error_id = int.from_bytes(data[-5:-3], byteorder='little')

    def bad_data(self, data):
        if len(data) < 271:
            return "Invalid length"

        elif data[0:4] != P.DATA_HEADER[0:4]:
            return "Invalid Header" 

        elif data[-3:] != P.DATA_FOOTER[0:3]:
            print()
            return "Invalid footer" 

        return 0 

    def __str__(self):
        return f'Data: {self.data.hex()}'
    


class FadSerialPacketHandler():
    '''
    Given an inital stream of data, a set of algorithms, and a byte size, this packet handler will feed
    new packets to the caller through either the next_packet or initial_packet function.
    '''

    PACKET_DUMP_SIZE = 250      # Sets the number of packets sent at a time

    def __init__(self, send_data, algorithms, byte_size):
        self.receive_buffer = []

        self.original_data = send_data
        self.data_size = P.UNSIGNED_BYTE_TYPES[byte_size]
        self.in_data = self.partition_data(send_data, byte_size)
        self.in_data_pos = -1 
        self.missed_packet_queue = queue.Queue()

        self.algorithms = algorithms

        self.first_packet = False   # keeps track of whether a first packet has been received
        self.message = ''
        self.finished_algorithm = True
        self.next_is_init = True
        self.dump_pos = 0

        self.current_algo = "None"

    def partition_data(self, data_to_send, byte_length):
        total_length = len(data_to_send)
        data_array = []
        array_vals_per_packet = P.PACKET_SIZE // byte_length
        num_packets = total_length * byte_length // P.PACKET_SIZE
        for i in range(num_packets):
            data_as_bytes = b''
            for j in range(array_vals_per_packet): # each int pos is equivalent to size
                data_pos = i * array_vals_per_packet + j
                data_as_bytes += data_to_send[data_pos].to_bytes(byte_length, byteorder='little')

            data_array.append(data_as_bytes)
        
        return data_array

    def add_packet(self, packet_data):
        ''' Add a received packet to the packet buffer for eventual display/handling'''
        try:
            packet = FadPacket(packet_data)
            # print("incoming:", packet.packets_incoming)

            if (len(self.receive_buffer) > 0):
                for i in range(self.receive_buffer[-1].packets_incoming - 1, packet.packets_incoming):
                    print(f"Missed packet {i}")
                    self.missed_packet_queue.put_nowait(i)

            self.receive_buffer.append(packet)

            if packet.packets_incoming == 0:
                self.display_received_packets()

        except TypeError as err:
            print(repr(err))
        
    
    def next_packet(self):
        '''Returns the next packet to be sent as a byte array, based on last packet sent. Returns None to force a pause or finish.'''
        packets_left = len(self.in_data) - self.in_data_pos - 1
        

        if self.in_data_pos == -1:
            self.in_data_pos = 0
            self._clear_data()
            return self._initial_packet()

        elif (packets_left < 0):
            self.in_data_pos = -1 # Prepare the next_packet function to send an initial packet next.
            self.dump_pos = 0
            self.message = f'Finished sending this set of packets for this algorithm.'
            return None

        elif self.dump_pos == self.PACKET_DUMP_SIZE: # For packet dumps (sending data in sets of packets instead of one big string of packets)
            self.dump_pos = 0

            remaining_dumps = ceil((len(self.in_data) - self.in_data_pos) / self.PACKET_DUMP_SIZE)

            self.message = f'Waiting on your mark to begin next dump, remaining: {remaining_dumps}'
            self.message += f'in data: {len(self.in_data)}, pos: {self.in_data_pos}'
            return None
        
        elif self.dump_pos == 0:
            self.message = ''

        try:
            missed_packet = self.missed_packet_queue.get_nowait()
        except queue.Empty:
            missed_packet = 0

        packet = self._create_generic_packet(self.in_data[self.in_data_pos], self.data_size, packets_left, missed_packet)
        self.in_data_pos += 1
        self.dump_pos += 1

        return packet


    def _initial_packet(self):
        '''Returns an initiation packet based on next algorithm choice'''

        if (len(self.algorithms) == 0):
            self.message = "Out of algorithms."
            self.in_data_pos = -1
            return None

        algo = self.algorithms.pop() 
        self.message = f'Starting next algorithm: {algo}'

        self.current_algo = algo

        data = algo + b'\0' * (P.PACKET_DATA_SIZE - len(algo))
        return self._create_generic_packet(data, P.ALGORITHM_SELECT, len(self.in_data), 0)

    def _create_generic_packet(self, data, data_type, packets_left, packets_missed):
            packet = P.DATA_HEADER
            packet += data_type
            packet += data
            packet += packets_left.to_bytes(2, byteorder='little')
            packet += packets_missed.to_bytes(2, byteorder='little')
            packet += P.DATA_FOOTER
            return packet

    def _clear_data(self):
        self.receive_buffer = []
        self.first_packet = False
    
    def save_data(self):
        algo = self.current_algo.decode('ascii')

        all_data = self.stitch_data()
        # np.savetxt(f, all_data)
        # np.savetxt(f, self.original_data)

        print(len(all_data))
        print(len(self.original_data))


        # np.savetxt(f'{str(datetime.datetime.now)[0:10]}{algo}.csv', a, delimiter=",")

        # f.close()
        
        
    def stitch_data(self):
        all_data = []   # Holds stitched data from each received packet
        for packet in self.receive_buffer:
            packet_data = list(packet.data)
            for data in packet_data:
                all_data.append(data * 16)
        return all_data

    def display_received_packets(self):
        # Stitch together data within packets
        all_data = self.stitch_data()

        dt.play(dt.center(all_data))
        dt.plot([all_data, self.original_data], labels=["Packet Data", "Input Data"])


def main():
    print(P.DATA_FOOTER)

if __name__ == "__main__":
    main()