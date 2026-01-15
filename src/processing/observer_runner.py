"""
test the effect of observer width on prediction quality
start with a width of 1, double until passed limit
"""
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
training_size = int(sys.argv[5])
step = int(sys.argv[6])
obs_hidden_layers = 2
train_epochs = 50

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

arts_file = h5.File(artifact_path, 'r')
data_file = h5.File(data_save_path, 'w')
arts = arts_file['features_PCA']

# normalize input
scaler = StandardScaler()
scaler.fit(arts)
arts = scaler.transform(arts)

sample_inds = np.linspace(training_size, len(arts) - training_size, samples, endpoint=False, dtype=int)

ls = []
ltls = []

ns = []
ntls = []

with tqdm(desc=f"{artifact_path}", total=len(sample_inds), unit="sample") as bar:
    for i in sample_inds:
        score, lookbacks, losses, training_losses = learnability(arts, i, lookback, step, training_size, train_epochs)
        ls.append(score)
        ltls.append(training_losses)

        score, losses, training_tosses = novelty(arts, i, lookback, step, training_size, training_size, train_epochs)
        ns.append(score)
        ntls.append(training_losses)

        bar.update()

data_file.create_dataset("learnability", data=ls)
data_file.create_dataset("novelty", data=ns)
data_file.create_dataset("training losses learnability", data=ltls)
data_file.create_dataset("training losses novelty", data=ntls)
data_file.close()
