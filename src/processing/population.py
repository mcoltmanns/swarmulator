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
pop = []

i = 0
for idx_pair in group_index:
    # get the real time for this log entry
    real_time = global_time_idx[i]
    i += 1
    time.append(real_time)

    # get the segment for this log entry from the state table
    start = idx_pair[0]
    length = idx_pair[1]
    segment = info[start: start + length]

    pop.append(len(segment))

    if i % 100 == 0:
        print(f'\t\r{i}/{len(group_index)}', end="")

plt.plot(time, pop)
plt.xlabel('time')
plt.ylabel('population')
plt.ylim(0, max(pop) + 10)
plt.title(sys.argv[4])
plt.savefig(sys.argv[3])
