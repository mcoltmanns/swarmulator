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
two different methods applied:
    method A concatenates all artifacts at a given timestep, then does PCA to shrink them back down to the original artifact size
    method B looks at the centroids from all object groups, and merges nearest neighbors until back down to the original artifact size
both methods have information loss measures, but they are not directly comparable
also a third method:
    method C simply concatenates artifacts across groups (no compression)

the old method for merging object type artifact series was to apply bag of words again over the artifact series, but this produced garbage
"""
import sys
import h5py
from sklearn.cluster import MiniBatchKMeans
from sklearn.decomposition import IncrementalPCA
import numpy as np
from tqdm import tqdm # progress bar

in_file = h5py.File(sys.argv[1], 'r')
out_file = h5py.File(sys.argv[2], 'w')

feature_count = int(sys.argv[3])

exclude_groups = sys.argv[4:len(sys.argv)] if len(sys.argv) > 3 else []
include_groups = []

global_time_idx = in_file['time']

artifacts_PCA = [] # array of sim artifacts
times = [] # map artifacts to their sim time

# minibatchkmeans lets us do batched processing (good for memory)
# but it can give biased results if we don't shuffle, so do passes (epochs) over each dataset, shuffling the chunks each time
batch_size = 50_000 # 50k is a solid number
epochs = 1 # tests show no drift after 1 epoch

# first step is to create per-object artifact series
# since we're analyzing the output of the swarm as a whole, we only want agent position, rotation, and signals (genes don't matter because those are not the output of the swarm, so we don't pull them in)
# NeuralAgent log entries are:
# id, parent id, position x, y, z, rotation x, y, z, signal a, signal b, energy, genes
# so for each entry in info we want entry[2:10] (indices should also be valid for anything that inherits neuralagent)
# these are the position, rotation, and signal of an agent
# and then partial fit the kmeans model

"""
is bag of words acceptable for feature extraction here?
- each log entry (agent log entry) is a "sentence"
- bag of words ignores the order of words in sentences, but this does not matter because the order of the data is the same over every sentence
the issue is that we have a variable length code for each time step (the raw data, concatenation of all live simobjects at that time)
but we want a fixed-length code per timestep
"""
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
    # fit the kmeans model
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
        for idx_pair in group_index:
            # get the segment for this log entry from the filtered state table
            start = idx_pair[0]
            length = idx_pair[1]
            # if log entry has no data, skip and append an empty artifact
            if length == 0:
                group_artifacts.append(np.zeros((feature_count)))
                print(f'no data at entry {start}')
            else:
                segment = info[start: start + length, 2:10] # all the log entries belonging to one time step
                # map each entry to a centroid, then count the number of times each centroid was mapped to
                clusters = kmeans.predict(segment)
                group_artifacts.append(np.bincount(clusters, minlength=feature_count))

            pbar.update()

    out_file.create_dataset(f"arts_{name}", data=group_artifacts) # ha ha farts
    out_file.create_dataset(f"centroids_{name}", data=kmeans.cluster_centers_)

# then we merge the per-object series into one master series
"""
method A: PCA merge
we have a known input size (feature vector width * object group count)
and a known output size (feature vector width)
so pca should not be so bad
do ipca
"""
# we fit a pca on the same number of components out as in and then save the cumsums which gives us a measure of how much information is lost for a given number of components
test_pca = IncrementalPCA(n_components=feature_count * len(include_groups), batch_size=batch_size)
pca = IncrementalPCA(n_components=feature_count, batch_size=batch_size)
batch_indices = list(range(0, out_file[f"arts_{include_groups[0]}"].shape[0], batch_size)) # batch indices are the same for all object groups
for start in tqdm(batch_indices, desc='Fitting diagnostic and real PCA', unit="batch"):
    end = min(start + batch_size, out_file[f"arts_{include_groups[0]}"].shape[0])
    batch = [[] for i in range(end - start)] # each batch entry should be abcd if we had two groups whose entries were ab and cd
    # within each batch, iterate over all object groups
    for name in include_groups:
        group_artifacts = out_file[f"arts_{name}"] # all artifacts in this batch for this object group
        group_batch = group_artifacts[start : end]
        for i in range(len(group_batch)):
            batch_art = group_batch[i] # grab one artifact from this group's batch
            for elem in batch_art:
                batch[i].append(elem) # append it to the aggregate artifact in the master batch
    test_pca.partial_fit(batch)
    pca.partial_fit(batch)
out_file.create_dataset('pca_cumsum', data=test_pca.explained_variance_ratio_.cumsum())
# also calculate at what reduction level we retain 90% or greater information
best_w = 1
for info_kept in test_pca.explained_variance_ratio_.cumsum():
    if info_kept < 0.9:
        best_w += 1
    else:
        break

# it's not much more work, so also fit a reduction that retains at least 90% of the information
pca_90 = IncrementalPCA(n_components=best_w, batch_size=batch_size)
artifacts_PCA_90 = []
for start in tqdm(batch_indices, desc=f'Fitting 90% accurate PCA (feature width {best_w})', unit="batch"):
    end = min(start + batch_size, out_file[f"arts_{include_groups[0]}"].shape[0])
    batch = [[] for i in range(end - start)]
    for name in include_groups:
        group_artifacts = out_file[f"arts_{name}"]
        group_batch = group_artifacts[start : end]
        for i in range(len(group_batch)):
            batch_art = group_batch[i]
            for elem in batch_art:
                batch[i].append(elem)
    pca_90.partial_fit(batch)

# now transform the data through the max compression and 90% pcas
# method C also happens here, since we aggregate artifacts anyways
artifacts_concat = []
for i in tqdm(range(out_file[f"arts_{include_groups[0]}"].shape[0]), desc="Transforming aggregate artifacts with PCA", unit="artifact"):
    agg_art = []
    for name in include_groups:
        group_artifact = out_file[f"arts_{name}"][i]
        for elem in group_artifact:
            agg_art.append(elem)
    artifacts_concat.append(agg_art)
    artifacts_PCA.append(pca.transform([agg_art])[0])
    artifacts_PCA_90.append(pca_90.transform([agg_art])[0])
    times.append(global_time_idx[i])

"""
Method B: centroid merging
centroids from different groups do not necessarily have the same dimension, and the dimensions do not necessarily encode the same things
so we can only merge centroids that belong to the same group

while the total number of centroids is greater than the goal artifact width:
    find the euclidean closest pair in each group
    from those pairs, take the closest
    merge the pair:
        create a new centroid that is the average of the pair
        at every timestep, assign the sum of the two artifact components belonging to the pair to the new centroid
        delete the old pair
"""
include_centroids = []
for name in include_groups:
    include_centroids.append(out_file[f'centroids_{name}'])

out_file.create_dataset('times', data=times)
out_file.create_dataset('features_direct', data=artifacts_concat)
out_file.create_dataset('features_PCA', data=artifacts_PCA)
out_file.create_dataset('features_PCA_90', data=artifacts_PCA_90)
print('Done')
out_file.close()
