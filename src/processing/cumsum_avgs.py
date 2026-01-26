import sys
import h5py as h5
import numpy as np

write = h5.File('/media/sda/pred-prey/cs_avgs.h5', 'w')

for i in range(3, 8):
    width = 2 ** i
    all_cs = []
    for a in range(1, 6):
        file = h5.File(f'/media/sda/pred-prey/pred_prey_{a}_arts_{width}.h5', 'r')

        this_cs = np.array(file['pca_cumsum'])
        all_cs.append(this_cs)

        file.close()

    write.create_dataset(f'width_{width}_avg', data=np.mean(all_cs, axis=0))
    write.create_dataset(f'width_{width}_avg_var', data=np.mean(np.var(all_cs, axis=0)))

write.close()
