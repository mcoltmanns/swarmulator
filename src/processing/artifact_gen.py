"""
Steps:
Fit k-means clustering model to the data (where k is the number of words in the bag)
Using the clustering model, assign every datapoint to its closest centroid.
Count the number of times each centroid was assigned to in each timestep - these are the feature vectors.
Save feature vectors to another H5 file.

args are:
    input file
    object group name in input file
    output file
    feature number
"""
import sys
import h5py
from sklearn.cluster import MiniBatchKMeans
import numpy as np
from tqdm import tqdm

in_file = h5py.File(sys.argv[1], 'r')
group = in_file['objects'][sys.argv[2]]
out_file = h5py.File(sys.argv[3], 'w')

feature_count = int(sys.argv[4])

global_time_idx = in_file['time']

info = group['state']['dynamic']
group_index = group['index']

artifacts = [] # array of sim artifacts
times = [] # map artifacts to their sim time

# minibatchkmeans lets us do batched processing (good for memory)
# but it can give biased results if we don't shuffle, so do three passes over each dataset, shuffling the chunks each time
batch_size = 50_000 # 50k is a solid number
epochs = 1 # tests show no drift after 1 epoch
kmeans = MiniBatchKMeans(n_clusters=feature_count, batch_size=batch_size)
batch_indices = list(range(0, info.shape[0], batch_size)) # indexes of the batches for training

# first step is to filter the data to only what we want
# since we're analyzing the output of the swarm as a whole, we only want agent position, rotation, and signals
# NeuralAgent log entries are:
# id, parent id, position x, y, z, rotation x, y, z, signal a, signal b, energy, genes
# so for each entry in info we want entry[2:10]
# and then partial fit the kmeans model
prev_centers = None
for epoch in range(epochs):
    np.random.shuffle(batch_indices) # shuffle the batch indices
    # fit on the current shuffle
    for start in tqdm(batch_indices, desc=f'Fitting (epoch {epoch}/{epochs})', unit="batch"):
        end = min(start + batch_size, info.shape[0])
        batch = info[start:end, 2:10]
        kmeans.partial_fit(batch)

    # calculate the drift since the last epoch
    if prev_centers is not None:
        drift = np.linalg.norm(kmeans.cluster_centers_ - prev_centers)
        print(f'\tDrift: {drift}')
    prev_centers = kmeans.cluster_centers_

with tqdm(total=len(group_index), desc="Generating artifacts", unit="entry") as pbar:
    i = 0
    for idx_pair in group_index: # lazy load
        # get real time for this log entry
        real_time = global_time_idx[i]
        i += 1
        times.append(real_time)

        # get the segment for this log entry from the filtered state table
        start = idx_pair[0]
        length = idx_pair[1]
        segment = info[start:start + length, 2:10] # also a lazy load

        clusters = kmeans.predict(segment)
        artifacts.append(np.bincount(clusters))

        pbar.update()

out_file.create_dataset('times', data=times)
out_file.create_dataset('features', data=artifacts)
print('Done')
out_file.close()
