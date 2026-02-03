import h5py as h5
import sys
import numpy as np

out_file = h5.File(sys.argv[1], 'w')
num = int(sys.argv[2])
feature_count = int(sys.argv[3])

# generate white noise artifacts for measure sanity tests
arts = np.random.uniform(low=-1, high=1, size=(num, feature_count))

out_file.create_dataset('features_PCA', data=arts)
