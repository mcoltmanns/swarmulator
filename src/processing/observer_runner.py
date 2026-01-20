import sys
import h5py as h5
import numpy as np
from tqdm import tqdm
import torch
from sklearn.preprocessing import StandardScaler

from observer_measures import learnability, novelty

artifact_path = sys.argv[1]
data_save_path = sys.argv[2]
lookback = int(sys.argv[3])
samples = int(sys.argv[4])
step = int(sys.argv[5])
train_epochs = 50

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

arts_file = h5.File(artifact_path, 'r')
data_file = h5.File(data_save_path, 'w')
arts = arts_file['features_PCA']

# normalize input
scaler = StandardScaler()
scaler.fit(arts)
arts = scaler.transform(arts)

chunk_starts, chunk_size = np.linspace(0, len(arts), samples, endpoint=False, dtype='int', retstep=True)

steps = list(range(step, step * 20, step))

with tqdm(desc=f"{artifact_path}", total=len(steps) * len(chunk_starts)) as bar:
    for s in steps:
        ls = []
        lls = []
        ltls = []

        ns = []
        nls = []
        ntls = []

        for chunk_start in chunk_starts:
            t = int(chunk_start + chunk_size / 2)
            print(f'sample from {chunk_start} to {chunk_start + chunk_size}, centerpoint {t}')
            print('learnability')
            score, lookbacks, losses, training_losses = learnability(arts, t, lookback, s, chunk_start, train_epochs)
            if score is not None and training_losses is not None:
                ls.append(score)
                lls.append(losses)
                ltls.append(training_losses)

            print('novelty')
            score, losses, training_tosses = novelty(arts, t, lookback, s, chunk_start, 5_000, train_epochs)
            if score is not None and training_losses is not None:
                ns.append(score)
                nls.append(losses)
                ntls.append(training_losses)

            bar.update()

        data_file.create_dataset(f"learnability_step{s}", data=ls)
        data_file.create_dataset(f"novelty_step{s}", data=ns)
        data_file.create_dataset(f"l_learnability_step{s}", data=lls)
        data_file.create_dataset(f"l_novelty_step{s}", data=nls)
        data_file.create_dataset(f"tl_learnability_step_{s}", data=ltls)
        data_file.create_dataset(f"tl_novelty_step_{s}", data=ntls)

data_file.create_dataset("chunk_start", data=chunk_starts)
data_file.create_dataset("chunk_size", data=chunk_size)
data_file.close()
