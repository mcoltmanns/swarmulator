import h5py as h5
import numpy as np
import matplotlib.pyplot as plt
import sys


def minmax_downsample(x, y, n_bins):
    bins = np.array_split(np.arange(len(y)), n_bins)
    idx = np.concatenate([
        [b[np.argmin(y[b])], b[np.argmax(y[b])]]
        for b in bins if len(b)
    ])
    idx.sort()
    return x[idx], y[idx]


in_file = h5.File(sys.argv[1], 'r')
y_name = sys.argv[2]
step = int(sys.argv[3])
samples = int(sys.argv[4])

data = np.array(in_file[y_name])
x = np.arange(0, len(data), dtype='int')
x *= step

x_ds, y_ds = minmax_downsample(x, data, samples)

plt.xlim(0, max(x))

plt.plot(x_ds, y_ds)
plt.show()
