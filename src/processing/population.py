import sys
import h5py as h5
from tqdm import tqdm

# args are <path to sim file>
# for every object under objects, builds a population time series

file = h5.File(sys.argv[1], 'a')

for name, group in file['objects'].items():
    try:
        info = group['state']['dynamic']
    except KeyError:
        continue
    if group.__contains__('pop'):
        continue
    group_index = group['index']

    pop = []

    i = 0
    for idx_pair in tqdm(group_index):
        # get the real time for this log entry
        i += 1

        # get the segment for this log entry from the state table
        start = idx_pair[0]
        length = idx_pair[1]
        segment = info[start: start + length]

        pop.append(len(segment))

    group.create_dataset('pop', data=pop)

file.close()
