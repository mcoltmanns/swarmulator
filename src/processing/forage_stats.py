import sys
import h5py as h5
from matplotlib import pyplot as plt
import math
import numpy as np
from scipy.spatial.distance import cdist

path = sys.argv[1]

file = h5.File(path, 'r')

# what do we want to know?
# average neighbor count
# population

global_time_idx = file['time']

forager_group = file['objects']['NeuralAgent'] # group with the foraging data
forager_info = forager_group['state']['dynamic']
forager_time_idx = forager_group['index']
forager_range = forager_group['state']['static'][0][0]

time = []
population = [] # array mapping log ids to population counts
avg_neighbor_count = []

i = 0
for idx_pair in forager_time_idx:
    # get the real time for this log entry
    real_time = global_time_idx[i]
    i += 1
    time.append(real_time)

    # get the segment for this log entry from the state table
    start = idx_pair[0]
    length = idx_pair[1]
    segment = forager_info[start : start + length]

    # population is just the number of entries in the segment (one per agent)
    population.append(len(segment))

    positions = np.array([row[2:5] for row in segment])
    # pairwise distances
    dists = cdist(positions, positions, metric='euclidean')
    # count neighbors in range
    neighbor_counts = np.sum((dists <= forager_range) & (dists > 0), axis=1)
    avg_neighbor_count.append(np.mean(neighbor_counts))

plt.plot(time, population)
plt.xlabel('time')
plt.ylabel('population')
plt.show()

plt.plot(time, avg_neighbor_count)
plt.xlabel('time')
plt.ylabel('average neighbor count')
plt.show()

