import esp_com as com
import serial



my_data = [400, 500, 700] * 400 

output_array = com.FadMonitor.partition_data(com.FadMonitor, my_data, 2)


for array in output_array:
    print("length: ", len(array))