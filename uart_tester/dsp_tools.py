import math
import numpy as np
from scipy import signal
import matplotlib.pyplot as plt
from random import random
import sounddevice as sd
import time
from audio2numpy import open_audio
from matplotlib.lines import Line2D


size = 4800



#A class that holds the signal arrays and their associated figure for a certain
#set of axes.
#while figure is being drawn, fill axes array with signal data. Init the instance with its figure.
class fig_signals:

    def __init__(self, figure):
        self.fig = figure
        self.axe_array = []
        self.signal_array = []
        print(self.fig)

    def add_axes(self, axe):
        self.axe_array.append(axe)

    def add_signal(self, signal):
        self.signal_array.append(signal)


def enter_axes(event):
    event.canvas.draw()
    for fig_signal in all_fig_signals:
        if(event.canvas.figure == fig_signal.fig):
            for i in range(len(fig_signal.axe_array)):
                if(event.inaxes == fig_signal.axe_array[i]):
                    print('playing audio...')
                    sd.stop()
                    sd.play(fig_signal.signal_array[i], 48000)
    

def normalize(array):
        #array = array / math.sqrt(np.sum(array**2))
        array = array / np.sum(array)
        return array




def normal_wave(in_pad, vals):
    out_signal = np.zeros(size)
    for x in range(size):
        if(x < in_pad):
            out_signal[x] = 0
        elif x < (in_pad + len(vals)):
            out_signal[x] = vals[x - in_pad]
        else:
            out_signal[x] = 0
    return out_signal

def noise(length = size):
    return np.random.random(length) - 0.5

def square_wave(wave_width, length = size):
    output = np.zeros(length)
    x = 0
    while(x < length):
        if (x // 2) % wave_width > 0:
            output[x] = 1
            x = x + 1
        else:
            x += wave_width
    return output - 0.5
            

        
    
def convolve(in_signal, impulse):

    out_signal = np.zeros(len(in_signal))
    
    for x in range(len(in_signal)):
        for y in range(len(impulse)):
            if (x + y) < size:
                out_signal[x + y] += in_signal[x] * impulse[y]
    return out_signal



def polarize(complex_array):
    return abs(complex_array), np.angle(complex_array)

# Compares the graphs of both the original given signals and their FFT information
def fft_compare(*signals, inverse = False, phase = False, title = ''):
    LINE_WIDTH = 0.5
    num_args = len(signals)
    
    fig, axes = plt.subplots(3, num_args)
    fig.suptitle(title)

    axes[0, 0].set_title("Input Signal before FFT")

    if phase:
        axes[1, 0].set_title("Magnitude of basis")
        axes[2, 0].set_title("Phase of basis")
    else:
        axes[1, 0].set_title("Cosine values of basis")
        axes[2, 0].set_title("Sine values of basis")
        
    for x in range(len(signals)):
        x_vals = np.linspace(0, len(signals[x]) - 1, len(signals[x]))

        # Plot the original signal on the 0'th row of the axes
        axes[0, x].plot(x_vals, signals[x], linewidth = LINE_WIDTH)
        
        fft_vals = np.fft.rfft(signals[x])
        half_vals = x_vals[:len(fft_vals)]

        # Plot the fft information in the next two axe rows with ...
        # ...Magnitude and Phase information
        if phase: 
            axes[1, x].plot(half_vals, abs(fft_vals), linewidth = LINE_WIDTH)
            
            axes[2, x].plot(half_vals, np.angle(fft_vals), linewidth = LINE_WIDTH)
        # ...or Real and Imagniary magnitudes
        else:
            axes[1, x].plot(half_vals, fft_vals.real, linewidth = LINE_WIDTH)
            
            axes[2, x].plot(half_vals, fft_vals.imag, linewidth = LINE_WIDTH)

            
# Creates an inverse impulse of the input signal, centered around the center value
def impulse_inverse(array, center = 0):
    array = array * -1
    array[center] += 1
    return array

# Creates a low pass filter of desired 
def low_pass_create(num_vals, length = size, filter_type = 'square'):
    # A square filter of length num_vals
    if filter_type == 'square':
        output = np.concatenate((np.ones(num_vals), np.zeros(length - num_vals)))

    # An exponential filter with a width of around num_vals
    elif filter_type == 'exp':
        output = np.zeros(length)
        for x in range(num_vals):
            if(x == length):
                break
            output[x] = math.exp(-7 * x / num_vals)

    # A sinc filter with a width of around num_vals
    elif filter_type == 'sinc':
        num_vals = num_vals + (num_vals % 2) - 1
        output = np.zeros(length)
        acc = 6
        for x in range(num_vals):
            if(x == num_vals // 2):
                output[x + (length - num_vals) // 2] = 1
            else:
                mult = ((x - num_vals // 2) / (num_vals // 2)) * acc * math.pi
                output[x + (length - num_vals) // 2] = math.sin(mult) / mult 

    return normalize(output)

# Applies a chosen low pass filter of length num to an input signal. If a custom filter is
# provided, num represents the 
def low_pass(signal, num, filter_type = 'square', offset = 0, custom_filter = None):

    if custom_filter:
        #in this case, num is useless
        np.convolve(signal, custom_filter[0])[offset:offset + len(signal)]

    elif filter_type == 'square':
        return np.convolve(signal, low_pass_create(num, len(signal)))[offset:offset + len(signal)]

    elif filter_type == 'exp':
        return np.convolve(signal, low_pass_create(num, len(signal), 'exp'))[offset:offset + len(signal)]

    elif filter_type == 'sinc':
        return np.convolve(signal, low_pass_create(num, num, filter_type = 'sinc'))[offset:offset + len(signal)]
     

def high_pass(signal, num, filter_type = 'square', offset = 0, custom_filer = None):
    return (signal, impulse_inverse(low_pass_create(num, len(signal))))[:len(signal)]
    

def de_echo(echo_wave, echo_distance):
    output_wave = np.copy(echo_wave)
    print(len(echo_wave) - echo_distance)
    for i in range(len(echo_wave) - echo_distance):
        output_wave[i + echo_distance] = output_wave[i + echo_distance] - output_wave[i]
    return output_wave

def fft_convolve(sig1, sig2):
    sig1_len = len(sig1)
    sig2_len = len(sig2)
    output = np.zeros(sig1_len + sig2_len - 1)
    i = 1
    while sig2_len > i:
        i = i * 2

    sig1_new = np.concatenate((sig1, np.zeros(sig2_len - 1)))
    sig2_new = np.concatenate((sig2, np.zeros(i - sig2_len)))
    sig2_fft = np.fft.rfft(sig2_new)

    j = 0
    tracker = []
    while j < sig1_len:
        sig = np.concatenate((sig1_new[j:j+(i // 2)], np.zeros(i // 2)))
        sig_combo = np.fft.rfft(sig) * sig2_fft
        combo_irfft = np.fft.irfft(sig_combo)
        tracker.append(combo_irfft)
        
        for k in range(i):
            output[j + k] = output[j + k] + combo_irfft[k]
        
        j = j + i//2
        
    return output, tracker
    

def get_song(time):

    fp = 'C:/ffmpeg/test_song_3.wav'

    song_signal, sampling_rate = open_audio(fp)

    trunc_signal = song_signal[1950000:1950000 + int(sampling_rate * time)]

    return trunc_signal

def signal_feeder(s, pll_func, buff_size = 1):
    output = []
    if(buff_size == 1):
        for i in range(len(s)):
            output.append(pll_func(s[i]))
        return output

    else:
        for i in range(len(s) // buff_size):
            temp = pll_func(s[i * buff_size:(i + 1) * buff_size])
            for j in range(buff_size):
                output.append(temp[j])
        return output

def plot(signals, labels = 0, title = "Figure"):
    fig, ax = plt.subplots()
    for s in signals:
        ax.plot(s)

    if(labels):
        ax.legend(labels)

    fig.suptitle(title)

def play(s):
    sd.play(s, 48000)

def show():
    plt.show()
    
#-------------------BEGIN PROGRAM-----------------------------#


#np.sum(array)
#my_array.sum()

#max()
#min()
#len(a)
#np.array([1, 3, 4], dtype = 'complex' or 'float32')

#np.arange(beginning = 0, end, step_size = 1)
#array = np.random.random(size)
#np.linspace(beginning, end, total_numbers) #inclusive
#irfft, ifft, etc. 

def main():
    print ("working")
    return


if __name__ == "__main__":
    main()
        
