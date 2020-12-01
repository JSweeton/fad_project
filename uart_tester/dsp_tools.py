import math
import numpy as np
from scipy import signal
import matplotlib.pyplot as plt
import sounddevice as sd
from audio2numpy import open_audio
from matplotlib.lines import Line2D


size = 4800
def to_discrete(np_array):
    '''Takes numpy array and turns it into array of ints'''
    return [int(i) for i in np_array]

def normalize(array):
    array = array / np.sum(array)
    return array

def center(array):
    array = array - np.average(array)
    array = array / (np.max(array) - np.min(array))
    return array

def flatten(array):
    array = array / (np.max(array) - np.min(array))
    array = array - np.min(array)
    return array

# def noise(length = size):
#     return np.random.random(length) - 0.5

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

def discrete_normal_wave(in_pad, vals):
    return to_discrete(normal_wave(in_pad, vals))

def square_wave(wave_width, length = size):
    output = np.zeros(length)
    x = 0
    while(x < length):
        if (x // 2) % wave_width > 0:
            output[x] = 1
            x = x + 1
        else:
            x += wave_width
    return output

def sine_wave(wave_width, length = size):
    ret = np.zeros(length)
    for i in range(length):
        ret[i] = 1 + math.sin(2 * math.pi * i / wave_width)
    return ret


def discrete_square_wave(wave_width, length):
    return to_discrete(square_wave(wave_width, length))


def convolve(in_signal, impulse):

    out_signal = np.zeros(len(in_signal))
    
    for x in range(len(in_signal)):
        for y in range(len(impulse)):
            if (x + y) < size:
                out_signal[x + y] += in_signal[x] * impulse[y]
    return out_signal

def polarize(complex_array):
    return abs(complex_array), np.angle(complex_array)

def fft_compare(*signals, inverse = False, phase = False, title = ''):
    '''Compares the graphs of both the original given signals and their FFT information'''

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

            
def impulse_inverse(array, center = 0):
    '''Creates an inverse impulse of the input signal, centered around the center value'''
    array = array * -1
    array[center] += 1
    return array

def low_pass_create(num_vals, length = size, filter_type = 'square'):
    '''Creates a low pass filter of desired filter width as num_vals and total length
        filter_type: square, exp, or sinc''' 

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

def low_pass(signal, num, filter_type = 'square', offset = 0, custom_filter = None):
    ''' Applies a chosen low pass filter of length num to an input signal. If a custom filter is
        provided, num is not used '''

    if custom_filter:
        #in this case, num is useless
        np.convolve(signal, custom_filter)[offset:offset + len(signal)]

    elif filter_type == 'square':
        return np.convolve(signal, low_pass_create(num, len(signal)))[offset:offset + len(signal)]

    elif filter_type == 'exp':
        return np.convolve(signal, low_pass_create(num, len(signal), 'exp'))[offset:offset + len(signal)]

    elif filter_type == 'sinc':
        return np.convolve(signal, low_pass_create(num, num, filter_type = 'sinc'))[offset:offset + len(signal)]
     

def high_pass(signal, num, filter_type = 'square', offset = 0, custom_filer = None):
    return (signal, impulse_inverse(low_pass_create(num, len(signal))))[:len(signal)]
    
def get_song(time):

    fp = 'C:/ffmpeg/test_song_3.wav'

    song_signal, sampling_rate = open_audio(fp)

    trunc_signal = song_signal[1950000:1950000 + int(sampling_rate * time)]

    return flatten(trunc_signal)

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

def plot_from_bytes(bytes):
    signals = [list(bytes)]
    plot(signals)

def plot(signals, labels, title: str = "Figure"):
    fig, ax = plt.subplots()
    for s in signals:
        ax.plot(s)

    if(labels):
        ax.legend(labels)

    fig.suptitle(title)

def play(s):
    import sounddevice as sd
    sd.play(s, 48000)

def show():
    plt.show()
    


def main():
    song = 100000 * get_song(30)
    print ("working")
    print(song[0:100])
    # sd.play(song)
    wave = square_wave(500, 500000)
    sd.play(wave)
    print("done")

if __name__ == "__main__":
    main()
        
