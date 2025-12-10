import sys
import h5py as h5
from matplotlib import pyplot as plt
from tqdm import tqdm

# args are <path to sim file> <path to save plot> <title of plot> <names of object groups of which you want to plot population>

file = h5.File(sys.argv[1], 'r')

global_time_idx = file['time']

max_pop = 0

for gname in sys.argv[4:]:
    group = file['objects'][gname]
    info = group['state']['dynamic']
    group_index = group['index']

    time = []
    pop = []

    i = 0
    for idx_pair in tqdm(group_index):
        # get the real time for this log entry
        real_time = global_time_idx[i]
        i += 1
        time.append(real_time)

        # get the segment for this log entry from the state table
        start = idx_pair[0]
        length = idx_pair[1]
        segment = info[start: start + length]

        pop.append(len(segment))

    max_pop = max(max_pop, max(pop))

    plt.plot(time, pop, label=gname, alpha=0.7, linewidth=0.3)

plt.xlabel('time')
plt.ylabel('population')
plt.legend(loc='upper right')
plt.ylim(0, max_pop + 10)
plt.title(sys.argv[3])
plt.savefig(sys.argv[2])
