from tools import dsp_tools as dt

import matplotlib.pyplot as plt


p = dt.get_talking(1)
p = p * 4000 + 2000
plt.plot(p)

plt.show()