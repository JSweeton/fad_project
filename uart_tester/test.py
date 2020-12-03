import esp_com as com
import serial
import dsp_tools as dt
import matplotlib.pyplot as plt


my_arr = dt.to_discrete(dt.square_wave(20, 512))

class fake:
    baudrate = 115200

fake_serial = fake()

mon = com.FadMonitor(serial_instance=fake_serial, send_data=my_arr, test_algorithms=[b'ALGO_WHITE_V1_0', b'ALGO_TEST'])


datum = [b'DATA\0U1\0\x12\0\x15', b'\x55\x14\0\x12' + b'\0' * 249, b'\0\0\0\0END\n']

for data in datum:
    # print(len(data))
    mon.handle_serial_input(data)


# ph = com.FadSerialPacketHandler(my_arr, [b'ALGO_WHITE_V1_0', b'ALGO_TEST'], 1)

# print(ph.initial_packet())
# print(ph.next_packet())
# print(ph.next_packet())
# print(ph.next_packet())

# print(ph.message)

# print(ph.initial_packet())