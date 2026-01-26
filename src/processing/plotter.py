import h5py as h5
import numpy as np
import matplotlib.pyplot as plt
import sys
from tqdm import tqdm

# args are <path to file> <x delta> <x name> <y name> <sample count> <raw start index> <raw end index> <paths to all data series to plot in file>
# plotted series must have same domains, and are absolute paths
# x delta is the amount the x axis changes between two data points
# this runs the time series through a kalman filter to make it a little easier on the eyes
# pass -1 to sample everything


def minmax_downsample(x, y, n_bins):
    bins = np.array_split(np.arange(len(y)), n_bins)
    idx = np.concatenate([
        [b[np.argmin(y[b])], b[np.argmax(y[b])]]
        for b in bins if len(b)
    ])
    idx.sort()
    return x[idx], y[idx]


in_file = h5.File(sys.argv[1], 'r')
step = int(sys.argv[2])
x_name = sys.argv[3]
y_name = sys.argv[4]
samples = int(sys.argv[5])
raw_start = int(sys.argv[6])
raw_end = int(sys.argv[7])
series = sys.argv[8:]

all_x = []
all_y = []
last_domain = None

for path in series:
    split = path.rfind('/')
    if split != 0:
        group = in_file[path[0:split]]
        data = group[path[split + 1:]][raw_start:raw_end]
    else:
        data = in_file[path[split + 1:]][raw_start:raw_end]
    domain = data.shape[0]
    if last_domain is not None and domain != last_domain:
        print('domains wrong')
        exit()
    x = np.arange(raw_start, raw_end, dtype='int')
    x *= step

    if samples > 0:
        x, data = minmax_downsample(x, data, samples)

    all_x.append(x)
    all_y.append(data)

for x, y in zip(all_x, all_y):
    plt.plot(x, y)

plt.xlim(min([min(x) for x in all_x]), max([max(x) for x in all_x]))

savename = f'{y_name}_v_{x_name}_{raw_start}-{raw_end}_s{samples}.pdf'
plt.savefig(savename, bbox_inches='tight')
print(f'saved at {savename}')
in_file.close()
