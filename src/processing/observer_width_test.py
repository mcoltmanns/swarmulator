"""
test the effect of observer width on prediction quality
start with a width of 1, double until passed limit
"""
import sys
import h5py as h5
import numpy as np
from tqdm import tqdm
import torch

from observer_metrics import novelty

artifact_path = sys.argv[1]
data_save_path = sys.argv[2]
lookback = int(sys.argv[3])
samples = int(sys.argv[4])
obs_max_width = int(sys.argv[5])
obs_hidden_layers = 2
train_epochs = 50

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

arts_file = h5.File(artifact_path, 'r')
data_file = h5.File(data_save_path, 'w')
arts = arts_file['features']
times = arts_file['times']

sample_inds = np.linspace(15_000, len(arts) - 15_000, samples, endpoint=False, dtype=int)
times_sampled = times[sample_inds]

widths = []
avg_losses = []
time_per_samples = []

with tqdm(total=obs_max_width) as pbar:
    current_obs_width = 8
    while current_obs_width <= obs_max_width:
        group = data_file.require_group(f'{current_obs_width}')
        width_total_loss = 0
        with tqdm(desc=f"width {current_obs_width}", total=len(sample_inds), unit="sample") as inner_bar:
            for i in sample_inds:
                novelty(arts, i, 20, 15_000, 50, 15_000, current_obs_width, group, device)
                final_training_loss = group[f"novelty_{i}_meta"]["training loss v epoch"][-1] # model loss after training at this time
                width_total_loss += final_training_loss
                inner_bar.update()
            time_per_samples.append(inner_bar.format_dict['elapsed'] / len(sample_inds))
        width_avg_loss = width_total_loss / len(sample_inds) # average loss after training across all samples for this width
        avg_losses.append(width_avg_loss)

        widths.append(current_obs_width)
        pbar.update(current_obs_width)
        current_obs_width *= 2

data_file.create_dataset("observer width", data=widths)
data_file.create_dataset("average loss over all samples vs width", data=avg_losses)
data_file.create_dataset("average sample processing time over all samples vs width", data=time_per_samples)
data_file.close()
