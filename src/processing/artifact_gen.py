"""
Steps:
Fit k-means clustering model to the data (where k is the number of words in the bag)
Using the clustering model, assign every datapoint to its closest centroid.
Count the number of times each centroid was assigned to in each timestep - these are the feature vectors.
Save feature vectors to another H5 file.

args are:
    input file
    output file
    feature number (how many features to create)
    any number of space-separated object group names to exclude from artifact generation

what if you have different object types in your swarm (several object type names, possibly with different width dynamic logs)?
apply one round of bag of words over each group to get uniform-length artifacts over all groups
then apply bag of words over those artifacts
"""
import sys
import h5py
from sklearn.cluster import MiniBatchKMeans
import numpy as np
from tqdm import tqdm # progress bar

in_file = h5py.File(sys.argv[1], 'r')
out_file = h5py.File(sys.argv[2], 'w')

feature_count = int(sys.argv[3])

exclude_groups = sys.argv[4:len(sys.argv)] if len(sys.argv) > 3 else []
include_groups = []

global_time_idx = in_file['time']

artifacts = [] # array of sim artifacts
times = [] # map artifacts to their sim time

# minibatchkmeans lets us do batched processing (good for memory)
# but it can give biased results if we don't shuffle, so do passes (epochs) over each dataset, shuffling the chunks each time
batch_size = 50_000 # 50k is a solid number
epochs = 1 # tests show no drift after 1 epoch

# first step is to create per-object artifact series
# since we're analyzing the output of the swarm as a whole, we only want agent position, rotation, and signals
# NeuralAgent log entries are:
# id, parent id, position x, y, z, rotation x, y, z, signal a, signal b, energy, genes
# so for each entry in info we want entry[2:10] (indices should also be valid for anything that inherits neuralagent)
# and then partial fit the kmeans model
for name, group in in_file['objects'].items(): # go over every group in the simulation
    if name in exclude_groups:
        print(name, "is excluded. Skipping")
        continue
    if 'dynamic' not in group['state'].keys():
        print(name, "has no dynamic state info. Skipping")
        continue
    include_groups.append(name)
    group_artifacts = []
    info = group['state']['dynamic'] # dynamic state information for this group over the course of the whole simulation
    group_index = group['index'] # group time indexing information
    kmeans = MiniBatchKMeans(n_clusters=feature_count, batch_size=batch_size)
    prev_centers = None
    batch_indices = list(range(0, info.shape[0], batch_size)) # start indices of the batches of a given size for this group
    for epoch in range(epochs):
        np.random.shuffle(batch_indices) # shuffle the batch indices
        # fit on the current shuffle
        for start in tqdm(batch_indices, desc=f'Fitting {name} (epoch {epoch}/{epochs})', unit="batch"):
            end = min(start + batch_size, info.shape[0])
            batch = info[start:end, 2:10]
            kmeans.partial_fit(batch)

        # calculate the drift since the last pass
        if prev_centers is not None:
            drift = np.linalg.norm(kmeans.cluster_centers_ - prev_centers)
            print(f'\tDrift: {drift}')
        prev_centers = kmeans.cluster_centers_

    with tqdm(total=len(group_index), desc=f"Generating artifacts {name}", unit="entry") as pbar:
        i = 0
        for idx_pair in group_index:
            # get the segment for this log entry from the filtered state table
            start = idx_pair[0]
            length = idx_pair[1]
            segment = info[start : start + length, 2:10] # all the log entries belonging to one time step
            # map each entry to a centroid, then count the number of times each centroid was mapped to
            clusters = kmeans.predict(segment)
            group_artifacts.append(np.bincount(clusters, minlength=feature_count))

            pbar.update()

    out_file.create_dataset(f"arts_{name}", data=group_artifacts) # ha ha farts

# then we merge the per-object series into one master series
# because the artifacts were created with different kmeans models, we have to fit a new one
# now we are down to one artifact per timestep, so we no longer need to batch by time
# this time around, the batches are constructed across the object groups
# FIXME something is wrong here. maybe it's better to take averages?
# does it even matter how these are combined?
epochs = 1 # more epochs here?
kmeans = MiniBatchKMeans(n_clusters=feature_count, batch_size=batch_size)
prev_centers = None
# so the batch size will be the number of object groups in the simulation, which won't be very big at all
# but we still have to do batched, because for very long simulations or large feature counts the total artifact array might be too large to load in memory
for name in include_groups:
    group_artifacts = out_file[f"arts_{name}"]
    batch_indices = list(range(0, group_artifacts.shape[0], batch_size))
    for epoch in range(epochs):
        np.random.shuffle(batch_indices)
        for start in batch_indices:
            end = min(start + batch_size, group_artifacts.shape[0])
            batch = group_artifacts[start:end]
            kmeans.partial_fit(batch)

        if prev_centers is not None:
            drift = np.linalg.norm(kmeans.cluster_centers_ - prev_centers)
            print(f'\tDrift: {drift}')
        prev_centers = kmeans.cluster_centers_

# then generate one aggregate artifact per timestep
with tqdm(total=len(global_time_idx), desc=f"Generating aggregate artifacts", unit="artifact") as pbar:
    i = 0
    for time in global_time_idx:
        ipt = []
        for gname in include_groups:
            ipt.append(out_file[f"arts_{gname}"][i])
        clusters = kmeans.predict(ipt)
        artifacts.append(np.bincount(clusters, minlength=feature_count))
        times.append(global_time_idx[i])
        i += 1

        pbar.update()

out_file.create_dataset('times', data=times)
out_file.create_dataset('features', data=artifacts)
print('Done')
out_file.close()

"""
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
    """
