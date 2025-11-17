import sys
import h5py as h5
from matplotlib import pyplot as plt

# args are <path to sim file> <name of object group for which you want the population> <path to save plot> <title of plot>

file = h5.File(sys.argv[1], 'r')

global_time_idx = file['time']

group = file['objects'][sys.argv[2]]
info = group['state']['dynamic']
group_index = group['index']

time = []
sig_a = []
sig_b = []

i = 0
for idx_pair in group_index:
    real_time = global_time_idx[i]
    i += 1
    time.append(real_time)

    start = idx_pair[0]
    length = idx_pair[1]
    segment = info[start:start+length]

    a = 0 # signal a is entry[8]
    b = 0 # signal b is entry[9]
    for entry in segment:
        a += entry[8]
        b += entry[9]

    a /= len(segment)
    b /= len(segment)
    sig_a.append(a)
    sig_b.append(b)

    if i % 100 == 0:
        print(f'\t\r{i}/{len(group_index)}', end="")

plt.plot(time, sig_a, label='A')
plt.plot(time, sig_b, label='B')
plt.legend()
plt.xlabel('time')
plt.ylim(0, 1.1)
plt.ylabel('average signal strength')
plt.title(sys.argv[4])
plt.savefig(sys.argv[3])
