import numpy as np
import h5py as h5
import sys
from tqdm import tqdm


def friction(deltas):
    if len(deltas) == 0:
        return 0.0
    # deltas is an array of changes in satisfaction
    return -(np.sum(deltas)) / float(len(deltas))


log_path = sys.argv[1]
data_save_path = sys.argv[2]
step = int(sys.argv[3])
max_pop = float(sys.argv[4])
exclude_objects = sys.argv[5:] if len(sys.argv) > 5 else []

in_file = h5.File(log_path, "r")
out_file = h5.File(data_save_path, "w")

avg_satisfactions = {} # the average agent satsifaction in each group at each timestep
group_satisfactions = {} # the group satisfaction in each group at each timestep (how successful this group is)
group_frictions = {} # the friction within each group at each timestep
system_satisfaction = [] # the system satisfaction at each timestep

for name, group in in_file['objects'].items():
    if name in exclude_objects:
        print(name, "is excluded. Skipping")
        continue
    if 'dynamic' not in group['state'].keys():
        print(name, 'has no dynamic state info. Skipping')
        continue

    info = group['state']['dynamic']
    group_index = np.array(group['index'])

    sample_inds = np.arange(0, len(group_index), step, dtype='int')
    group_index = group_index[sample_inds]

    avg_satisfactions[name] = np.zeros((len(group_index)))
    group_satisfactions[name] = np.zeros((len(group_index)))
    group_frictions[name] = np.zeros((len(group_index)))
    group_max_pop = -np.inf

    last_entries = None

    with tqdm(total=len(group_index), desc=f'calculating satisfaction for {name}', unit='entry') as bar:
        for i, [start, length] in enumerate(group_index):
            entries = np.array(info[start:start + length])

            # reproduction threshold is 8, agent energy is index 10 of their log
            # so an agent's satisfaction is entry[10] / 8.0
            if name == 'Plant':
                segment_satisfactions = entries[:, 6] / 25.0
            else:
                segment_satisfactions = entries[:, 10] / 8.0

            avg_satisfactions[name][i] = np.mean(segment_satisfactions)

            # find agent deltas
            # first we have to find all the entries between now and the last step where the agent id (index 0) is the same
            # this only works if the sample interval is small enough
            if last_entries is not None and name != 'Plant':
                deltas = []
                current_ids = entries[:, 0]
                last_ids = last_entries[:, 0]

                c_order = np.argsort(current_ids)
                c_sorted = entries[c_order]
                c_ids_sorted = current_ids[c_order]

                # indices of current ids locations in the array of last ids
                idx = np.searchsorted(c_ids_sorted, last_ids)

                mask = idx < len(c_ids_sorted)
                mask[mask] &= c_ids_sorted[idx[mask]] == last_ids[mask]

                last_matched = last_entries[mask]
                current_matched = c_sorted[idx[mask]]

                for current, last in zip(current_matched, last_matched):
                    deltas.append(current[10] - last[10])

                group_frictions[name][i] = friction(deltas)

            last_entries = entries

            # save data for group satisfaction calc
            group_satisfactions[name][i] = length
            group_max_pop = max(group_max_pop, length)

            # save data for system satisfaction calc
            if len(system_satisfaction) <= i:
                system_satisfaction.append(float(length))
            else:
                system_satisfaction[i] += float(length)

            bar.update()

    # take group satisfaction as the current population over its maximum
    group_satisfactions[name] = np.array(group_satisfactions[name]) / float(group_max_pop)

# take system satisfaction as the current total population over its maximum
system_satisfaction = np.array(system_satisfaction) / max_pop

for k, v in avg_satisfactions.items():
    out_file.create_dataset(f'{k}_avg_satisfaction', data=v)
for k, v in group_satisfactions.items():
    out_file.create_dataset(f'{k}_group_satisfaction', data=v)
for k, v in group_frictions.items():
    out_file.create_dataset(f'{k}_internal_friction', data=v)
out_file.create_dataset('system_satisfaction', data=system_satisfaction)

# now calculate intergroup and group-system frictions
# for this we take the deltas of the group satisfactions, since we are comparing friction between groups
for name_a, sats_a in group_satisfactions.items():
    # intergroup frictions
    for name_b, sats_b in group_satisfactions.items():
        igf = []
        # friction between two things is (-delta_a - delta_b) / 2
        # does taking the igf between a group and itself make sense?
        # since the satisfactions are averages, i think it does - it is a measure of how synergistic or competitive the group is
        last_a, last_b = None, None

        for a, b in zip(sats_a, sats_b):
            if last_a is not None and last_b is not None:
                delta_a = a - last_a
                delta_b = b - last_b
                igf.append(friction([delta_a, delta_b]))
            last_a = a
            last_b = b

        out_file.create_dataset(f'{name_a}-{name_b}_friction', data=igf)

    # group-system frictions
    gsf = []

    last_g, last_s = None, None

    for g, s in zip(sats_a, system_satisfaction):
        if last_g is not None and last_s is not None:
            delta_g = g - last_g
            delta_s = s - last_s
            gsf.append(friction([delta_g, delta_s]))
        last_g = g
        last_s = s

    out_file.create_dataset(f'system-{name_a}_friction', data=gsf)

out_file.close()
