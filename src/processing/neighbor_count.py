import sys
import h5py as h5
import numpy as np
from scipy.spatial.distance import cdist
from tqdm import tqdm

# args are <path to sim file> <names of object groups to calculate for>
# neighbors are calculated only within object groups

file = h5.File(sys.argv[1], 'a')

max_neighbors = 0

for gname in sys.argv[2:]:
    group = file['objects'][gname]
    info = group['state']['dynamic']
    group_index = group['index']
    group_range = group['state']['static'][0][0]

    avg_neighbor_count = []

    i = 0
    for idx_pair in tqdm(group_index):
        # get the real time for this log entry
        i += 1

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

    group.create_dataset('avg_neighbor_count', data=avg_neighbor_count)

file.close()
