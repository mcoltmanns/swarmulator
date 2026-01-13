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

    # slightly faster with batching on ssd, expect big batch improvement on hdd
    with tqdm(total=len(group_index), desc=f"Generating artifacts {name}", unit="entry") as pbar:
        for batch_start in batch_indices:
            batch_end = min(batch_start + batch_size, group_index.shape[0])
            for idx_pair in group_index[batch_start:batch_end]:
                # get the segment for this log entry from the filtered state table
                start = idx_pair[0]
                length = idx_pair[1]
                # if log entry has no data, skip and append an empty artifact
                if length == 0:
                    group_artifacts.append(np.zeros((feature_count)))
                else:
                    segment = info[start: start + length, 2:10] # all the log entries belonging to one time step
                    # map each entry to a centroid, then count the number of times each centroid was mapped to
                    clusters = kmeans.predict(segment)
                    group_artifacts.append(np.bincount(clusters, minlength=feature_count))

                pbar.update()

    out_file.create_dataset(f"arts_{name}", data=group_artifacts) # ha ha farts
    out_file.create_dataset(f"centroids_{name}", data=kmeans.cluster_centers_)

# after this step each object group has a fixed-width time series of artifacts associated with it
# the widths of all the artifacts across all the groups are the same
# then we merge the per-object series into one master series
"""
method A: PCA merge
we have a known input size (feature vector width * object group count)
and a known output size (feature vector width)
so pca should not be so bad
do ipca
"""
# we fit a pca on the same number of components out as in and then save the cumsums which gives us a measure of how much information is lost for a given number of components
# pca for finding information retention
test_pca = IncrementalPCA(n_components=feature_count * len(include_groups), batch_size=batch_size)
# pca for the actual transformation
pca = IncrementalPCA(n_components=feature_count, batch_size=batch_size)
batch_indices = list(range(0, out_file[f"arts_{include_groups[0]}"].shape[0], batch_size)) # batch indices are the same for all object groups
for start in tqdm(batch_indices, desc='Fitting diagnostic and real PCA', unit="batch"):
    end = min(start + batch_size, out_file[f"arts_{include_groups[0]}"].shape[0])
    batch = [[] for i in range(end - start)] # each batch entry should be abcd if we had two groups whose entries were ab and cd
    # within each batch, iterate over all object groups
    for name in include_groups:
        group_artifacts = out_file[f"arts_{name}"]
        group_batch = group_artifacts[start:end] # all artifacts in this batch for this object group
        for i in range(len(group_batch)): # iterate over the artifacts in the batch for this object group
            batch_art = group_batch[i] # grab one artifact from this group's batch
            for elem in batch_art:
                batch[i].append(elem) # append it to the aggregate artifact at the corresponding index in the master batch
    test_pca.partial_fit(batch) # fit the pca to the batch
    pca.partial_fit(batch)
out_file.create_dataset('pca_cumsum', data=test_pca.explained_variance_ratio_.cumsum())
# also calculate at what reduction level we retain 90% or greater information
best_w = 1
for info_kept in test_pca.explained_variance_ratio_.cumsum():
    if info_kept < 0.9:
        best_w += 1
    else:
        break

# it's not much more work, so also fit a reduction that retains at least 90% of the information based on data from the test pca
pca_90 = IncrementalPCA(n_components=best_w, batch_size=batch_size)
artifacts_PCA_90 = []
for start in tqdm(batch_indices, desc=f'Fitting 90% accurate PCA (feature width {best_w})', unit="batch"):
    end = min(start + batch_size, out_file[f"arts_{include_groups[0]}"].shape[0])
    batch = [[] for i in range(end - start)]
    for name in include_groups:
        group_artifacts = out_file[f"arts_{name}"]
        group_batch = group_artifacts[start:end]
        for i in range(len(group_batch)):
            batch_art = group_batch[i]
            for elem in batch_art:
                batch[i].append(elem)
    pca_90.partial_fit(batch)

# now transform the data through the max compression and 90% pcas
# method C also happens here, since we aggregate artifacts anyways
artifacts_concat = []
for start in tqdm(batch_indices, desc='Applying PCA to aggregate artifacts', unit="batch"):
    end = min(start + batch_size, out_file[f'arts_{include_groups[0]}'].shape[0])
    agg_arts = [[] for i in range(end - start)]
    for name in include_groups:
        group_batch = out_file[f'arts_{name}'][start:end]
        for i in range(len(group_batch)):
            for elem in group_batch[i]:
                agg_arts[i].append(elem)
    artifacts_concat.extend(agg_arts)
    artifacts_PCA.extend(pca.transform(agg_arts))
    artifacts_PCA_90.extend(pca_90.transform(agg_arts))
    times.append(global_time_idx[i])

# dump to disk asap to save memory with large datasets
out_file.create_dataset('times', data=times)
out_file.create_dataset('features_direct', data=artifacts_concat)
out_file.create_dataset('features_PCA', data=artifacts_PCA)
out_file.create_dataset('features_PCA_90', data=artifacts_PCA_90)

"""
Method B: centroid merging
centroids from different groups do not necessarily have the same dimension, and the dimensions do not necessarily encode the same things
so we can only merge centroids that belong to the same group

BUT - while that is true, we can safely lift lower dimensional centroids into the space of the highest-dimension centroid by just padding them with zeros
i think this is the best solution
    - allows "fair" compression - impossible for an object group to get compressed down to only one centroid
    - padding with zeros is a simple, bijective transformation, so i feel like it wouldn't introduce too much extra information

while the total number of centroids is greater than the goal artifact width:
    find the euclidean closest pair in each group
    from those pairs, take the closest
    merge the pair:
        create a new centroid that is the average of the pair
        at every timestep, assign the sum of the two artifact components belonging to the pair to the new centroid
        delete the old pair

what about the information loss measures?
- we said the interestingness of a centroid group was its average distance from its center
- probably also take variance to be safe
- again, for centroids of different dimensions, lift the lower dimension ones with 0 padding
"""


# lift all points in an array of points to the same dimension by padding with zeros
def lift(points):
    dim = max([len(point) for point in points])
    lifted = []
    for point in points:
        lifted.append(np.pad(point, ((0, dim - len(point))), mode='constant', constant_values=0))
    return lifted


# calculate the interestingnesses of an array of points
def interestingness(points):
    # the interestingness of a point is the average of its euclidean distances from all other points (because we assume that points which are nearby encode for similar things)
    res = []
    for point in points:
        dist_sum = 0
        for other in points:
            dist_sum += distance(point, other)
        dist_sum /= len(points)
        res.append(dist_sum)
    return res


# average position of given positions in an array
def average_position(indices, positions):
    dim = len(positions[0])
    avg = [0.0] * dim

    for i in indices:
        for d in range(dim):
            avg[d] += positions[i][d]

    n = len(indices)
    return [x / n for x in avg]


def distance(a, b):
    a = np.array(a)
    b = np.array(b)
    return np.linalg.norm(a - b)


# gather all the centroids and lift them to the same dimension
centroids = []
for name in include_groups:
    for centroid in out_file[f'centroids_{name}']:
        centroids.append(centroid)
centroids = lift(centroids)

# save the lifted centroids
out_file.create_dataset('centroids_direct', data=centroids)

# tracking arrays for the centroid merge
# positions of merged centroids
merged_centroids = [c for c in centroids]
# for merged centroid i, track which original centroids it contains (by index)
groups = [{i} for i in range(len(centroids))]

while len(merged_centroids) > feature_count:
    # find nearest centroids
    min_dist = np.inf
    c_i, c_j = None, None

    for i in range(len(merged_centroids)):
        for j in range(i + 1, len(merged_centroids)):
            d = distance(merged_centroids[i], merged_centroids[j])
            if d < min_dist:
                min_dist = d
                c_i, c_j = i, j

    # calculate the new center of this group
    new_centroid = average_position([c_i, c_j], merged_centroids)
    # merge the group membership sets
    new_members = groups[c_i] | groups[c_j]

    # remove old centroids, add new centroid
    # have to iterate backwards so as to not invalidate indices
    for i in sorted([c_i, c_j], reverse=True):
        del merged_centroids[i]
        del groups[i]

    merged_centroids.append(new_centroid)
    groups.append(new_members)

# now merged_centroids[k] gives the position of merged centroid k
# groups[k] gives the indices of all original centroids in merged centroid k
# build a reverse map: original centroid indices to merged indices
groups_reversed = {}
for new_index, members in enumerate(groups):
    for original_index in members:
        groups_reversed[original_index] = new_index

print(merged_centroids)
print(groups)
print(groups_reversed)

# save the merged centroids
out_file.create_dataset("centroids_merged", data=merged_centroids)

# now merge the artifacts
merged_artifacts = []
with tqdm(total=out_file['features_direct'].shape[0], desc='Generating merged-centroid artifacts', unit='artifact') as pbar:
    for start in batch_indices:
        end = min(start + batch_size, out_file['features_direct'].shape[0])
        centroid_index = 0
        batch = out_file['features_direct'][start:end]
        for artifact in batch:
            # empty merged artifact
            merged_artifact = [0] * feature_count
            for i in range(len(artifact)):
                # add the count on the old centroid to the merged centroid in the new artifact
                merged_artifact[groups_reversed[i]] += artifact[i]
            merged_artifacts.append(merged_artifact)
            pbar.update()

out_file.create_dataset("features_merged", data=merged_artifacts)
out_file.create_dataset("centroids_direct_interestingness", data=interestingness(centroids))
out_file.create_dataset("centroids_merged_interestingness", data=interestingness(merged_centroids))
out_file.create_dataset("centroids_direct_interestingness_avg", data=np.mean(interestingness(centroids)))
out_file.create_dataset("centroids_merged_interestingness_avg", data=np.mean(interestingness(merged_centroids)))

print('Done')
out_file.close()
