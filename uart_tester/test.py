import esp_com as com
import serial


# Test handle serial function

packet = [b'DATA', b'\x00\x10', b'abcdeabcde', b'ENDSIG']

my_data = com.FadSerialPacket(packet)

print(my_data.data.hex())