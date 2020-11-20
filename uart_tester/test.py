import esp_com as com
import serial
import dsp_tools
import matplotlib.pyplot as plt


my_arr = dsp_tools.square_wave(20, 256)


plt.plot(my_arr)
plt.show()