import sys
import h5py as h5
from matplotlib import pyplot as plt
import numpy as np
from scipy.spatial.distance import cdist

# args are <path to sim file> <name of object group for which you want the average neighbor count> <path to save plot> <title of plot>

file = h5.File(sys.argv[1], 'r')

# what do we want to know?
# average neighbor count
# population

global_time_idx = file['time']

group = file['objects'][sys.argv[2]] # group with the foraging data
info = group['state']['dynamic']
group_index = group['index']
group_range = group['state']['static'][0][0]

time = []
avg_neighbor_count = []

i = 0
for idx_pair in group_index:
    # get the real time for this log entry
    real_time = global_time_idx[i]
    i += 1
    time.append(real_time)

    # get the segment for this log entry from the state table
    start = idx_pair[0]
    length = idx_pair[1]
    segment = info[start : start + length]

    positions = np.array([row[2:5] for row in segment])
    # pairwise distances
    dists = cdist(positions, positions, metric='euclidean')
    # count neighbors in range
    neighbor_counts = np.sum((dists <= group_range) & (dists > 0), axis=1)
    avg_neighbor_count.append(np.mean(neighbor_counts))

    if i % 100 == 0:
        print(f'\t\r{i}/{len(group_index)}', end="")

plt.plot(time, avg_neighbor_count)
plt.xlabel('time')
plt.ylabel('average neighbor count')
plt.ylim(0, max(avg_neighbor_count) + 1)
plt.title(sys.argv[4])
plt.savefig(sys.argv[3])
