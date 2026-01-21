import h5py as h5
import numpy as np
import matplotlib.pyplot as plt
import sys
from filterpy.kalman import KalmanFilter
from tqdm import tqdm

# args are <path to file> <number of samples> <x delta> <x name> <y name> <raw start index> <raw end index> <paths to all data series to plot in file>
# plotted series must have same domains, and are absolute paths
# x delta is the amount the x axis changes between two data points
# this runs the time series through a kalman filter to make it a little easier on the eyes


def minmax_downsample(x, y, n_bins):
    bins = np.array_split(np.arange(len(y)), n_bins)
    idx = np.concatenate([
        [b[np.argmin(y[b])], b[np.argmax(y[b])]]
        for b in bins if len(b)
    ])
    idx.sort()
    return x[idx], y[idx]


in_file = h5.File(sys.argv[1], 'r')
samples = int(sys.argv[2])
step = int(sys.argv[3])
x_name = sys.argv[4]
y_name = sys.argv[5]
raw_start = int(sys.argv[6])
raw_end = int(sys.argv[7])
series = sys.argv[8:]

all_x = []
all_y = []
last_domain = None

for path in series:
    split = path.rfind('/')
    print(path[0:split])
    print(path[split + 1:])
    group = in_file[path[0:split]]
    data = group[path[split + 1:]][raw_start:raw_end]
    domain = data.shape[0]
    if last_domain is not None and domain != last_domain:
        print('domains wrong')
        exit()
    x = np.arange(raw_start, raw_end, dtype='int')
    x *= step

    burnin = int(len(data) * 0.1)
    print(f'burnin {burnin}')
    f = KalmanFilter(dim_x=1, dim_z=1) # univariate time series
    f.F = np.array([[1.]])
    f.H = np.array([[1.]])

    # fit the filter
    q_var = np.var(data[:burnin])
    f.Q = np.array([[q_var]])
    f.R = np.array([[q_var]])

    x_rest = list()
    f.x = np.array([0])
    f.P *= 10
    y_test = data[burnin:]
    for y in tqdm(y_test[:-1]):
        f.predict()
        x_rest.append(f.x[0])
        f.update([y])

    y_filtered = [0] + list(data[:burnin]) + x_rest

    all_x.append(x)
    all_y.append(y_filtered)

for x, y in zip(all_x, all_y):
    plt.xlim(min(x), max(x))
    plt.plot(x, y)
plt.show()
in_file.close()
