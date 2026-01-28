import sys
import h5py as h5
from tqdm import tqdm

# args are <path to sim file> <path to save plot> <title of plot> <names of object groups for which you want the population>

file = h5.File(sys.argv[1], 'a')

for gname in sys.argv[2:]:
    group = file['objects'][gname]
    info = group['state']['dynamic']
    group_index = group['index']

    sig_a = []
    sig_b = []

    i = 0
    for idx_pair in tqdm(group_index):
        i += 1

        start = idx_pair[0]
        length = idx_pair[1]
        segment = info[start:start+length]

        a = 0 # signal a is entry[8]
        b = 0 # signal b is entry[9]
        for entry in segment:
            a += entry[8]
            b += entry[9]

        a /= len(segment)
        b /= len(segment)
        sig_a.append(a)
        sig_b.append(b)

    group.create_dataset('sig_a', data=sig_a)
    group.create_dataset('sig_b', data=sig_b)

file.close()
