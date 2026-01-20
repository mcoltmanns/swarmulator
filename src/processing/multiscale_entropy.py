import h5py as h5
import numpy as np
# remember to cite entropyhub!
import EntropyHub as eh
import sys
import time


def mvmse(artifacts):
    params = eh.MSobject('MvPermEn') # gets undefined entropy values with sampleentropy
    # find out - why is mv permutation entropy the measure we want here? see Ahmed & Mandic 2011
    # sample 10 complexity scales, although we could probably do more. number of scales doesn't seem to affect runtime.
    # what space are the scales in? i think temporal
    entropy, ci = eh.MvMSEn(artifacts, params, Scales=20)

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
arts_sampled = arts[sample_inds]

print(arts_sampled.shape)
start = time.time()
entropy, complexity_idx = mvmse(arts_sampled)
print(round(time.time() - start), 'seconds')
g = data_file.require_group(data_save_name)
g.create_dataset("entropy", data=entropy)
g.create_dataset('complexity index', data=complexity_idx)
data_file.close()
