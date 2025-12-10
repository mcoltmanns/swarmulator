import sys
import h5py as h5
from matplotlib import pyplot as plt
import numpy as np
from scipy.spatial.distance import cdist
from tqdm import tqdm

# args are <path to sim file> <path to save plot> <title of plot> <names of object groups for which you want the average neighbor count>
# neighbors are calculated only within object groups

file = h5.File(sys.argv[1], 'r')

global_time_idx = file['time']

max_neighbors = 0

for gname in sys.argv[4:]:
    group = file['objects'][gname]
    info = group['state']['dynamic']
    group_index = group['index']
    group_range = group['state']['static'][0][0]

    time = []
    avg_neighbor_count = []

    i = 0
    for idx_pair in tqdm(group_index):
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

    max_neighbors = max(max_neighbors, max(avg_neighbor_count))

    plt.plot(time, avg_neighbor_count, label=gname, alpha=0.7, linewidth=0.3)

plt.xlabel('time')
plt.ylabel('average neighbor count')
plt.legend(loc='upper right')
plt.ylim(0, max_neighbors + 1)
plt.title(sys.argv[3])
plt.savefig(sys.argv[2])
