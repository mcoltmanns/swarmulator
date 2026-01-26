import h5py as h5
import numpy as np
# remember to cite entropyhub!
import EntropyHub as eh
import sys
import time


def mvmse(artifacts, en_type):
    params = eh.MSobject(en_type)
    # find out - why is mv permutation entropy the measure we want here? see Ahmed & Mandic 2011
    # sample 10 complexity scales, although we could probably do more. number of scales doesn't seem to affect runtime.
    # what space are the scales in? i think temporal
    # the scales are temporal. scale 1 is every step, scale 2 every other, etc.
    # because of our sampling, base skip is 30
    # with a max scale of 200, skip is 30*200 = 6000 / 500000 = only 1.2% of the simulation

    # for sample entropy:
    # from ahmed/mandic - a monotonically decreasing entropy means the time series only contains information at the smalles scales, typical of either completely random or completely predictable series
    # A multivariate system exhibiting long-range correlations and complex generating dynamics is characterized by either a constant multivariate sample entropy or it exhibits a monotonic increase in multivariate sample entropy with the scale factor.

    # permutation entropy measures the number of unique orderings of state we see within the sliding time window
    # many orderings of state at low scale means there are many different behaviors when viewed at a small timescale
    # few orderings of state at high scale means there are few different behaviors at a large timescale
    # that's what we expect from anything really - correlate to sigma parameter

    # sample entropy more or less measures the similarity of patterns between vectors
    # so a low sample entropy means that there are many similar patterns / many similar versions of the state space
    # a high sample entropy means there are few similarities
    # probably for the kind of open-endedness we want we would want entropy to increase with scale - the agents aren't evolving new capabilities, but on the timescale of the swarm we want to see new arrangements

    # we have to normalize artifacts for entropy to work
    norm = np.linalg.norm(artifacts, axis=1)
    arts_normed = artifacts / norm[:, np.newaxis]

    entropy, ci = eh.MvMSEn(arts_normed, params, Scales=20, Plotx=False)

    return entropy, ci


artifact_path = sys.argv[1]
data_save_path = sys.argv[2]
samples = 16384
data_save_name = sys.argv[3]

arts = h5.File(artifact_path, 'r')['features_PCA']
data_file = h5.File(data_save_path, 'a')

# according to the following snippet, 16384 samples of the space seems to be good.
# the analysis takes just about 200 seconds, and the spacing between the samples comes out to 30 log steps
# which is 30*20 = 600 real steps, which is 30/10000000=0.003% of the simulation. plenty fine-grained!
# 10 minutes longest runtime
# tenmins = 60 * 10
# deltat = 0
# sample_count = 256
# while deltat < tenmins:
#     sample_inds = np.linspace(0, len(arts), num=sample_count, dtype=np.int32, endpoint=False)
#     arts_sampled = arts[sample_inds]
#     start = time.time()
#     mvmse(arts_sampled)
#     deltat = time.time() - start
#     print(f'{sample_count} samples took {round(deltat)} seconds, spacing between samples was {sample_inds[1] - sample_inds[0]}')
#     sample_count *= 2

sample_inds = np.linspace(0, len(arts), num=samples, dtype=np.int32, endpoint=False)
arts_sampled = np.array(arts[sample_inds])

print('sample')
start = time.time()
entropy, complexity_idx = mvmse(arts_sampled, 'MvSampEn')
print(round(time.time() - start), 'seconds')
g = data_file.require_group(data_save_name)
g.create_dataset("sample entropy", data=entropy)
g.create_dataset('sample complexity index', data=complexity_idx)

print('permutation')
start = time.time()
entropy, complexity_idx = mvmse(arts_sampled, 'MvPermEn')
print(round(time.time() - start), 'seconds')
g = data_file.require_group(data_save_name)
g.create_dataset("sample entropy", data=entropy)
g.create_dataset('sample complexity index', data=complexity_idx)
data_file.close()
