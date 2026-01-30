import numpy as np
import h5py as h5
import matplotlib.pyplot as plt
import sys

base_path = sys.argv[1]

for scenario in ['forage', 'pd', 'pred-prey']:
    l_vars = np.zeros((5,))
    n_vars = np.zeros((5,))
    nd_vars = np.zeros((5,))
    #na_vars = np.zeros((5,))
    x = np.zeros((5,))
    count = 0
    for i in range(1, 6):
        for s in range(4, 8):
            step = 2 ** s
            for e in range(3, 8):
                width = 2 ** e
                if scenario == 'pred-prey':
                    normal_measures = h5.File(f'{base_path}/{scenario}/pred_prey_{i}_observer_w{width}.h5', 'r')
                    decay_measures = h5.File(f'{base_path}/{scenario}/pred_prey_{i}_observer_decay_w{width}.h5', 'r')
                    #avg_measures = h5.File(f'{base_path}/{scenario}/pred_prey_{i}_observer_avg_w{width}.h5', 'r')
                else:
                    normal_measures = h5.File(f'{base_path}/{scenario}/{scenario}_{i}_observer_w{width}.h5', 'r')
                    decay_measures = h5.File(f'{base_path}/{scenario}/{scenario}_{i}_observer_decay_w{width}.h5', 'r')
                    #avg_measures = h5.File(f'{base_path}/{scenario}/{scenario}_{i}_observer_avg_w{width}.h5', 'r')

                # variances of the given scores for the given run, step, and width
                l_var = np.var(normal_measures[f'learnability_step{step}'])
                n_var = np.var(normal_measures[f'novelty_step{step}'])
                nd_var = np.var(decay_measures[f'novelty_step{step}'])
                #na_var = np.var(avg_measures[f'novelty_step{step}'])

                l_vars[e - 3] += (l_var)
                n_vars[e - 3] += (n_var)
                nd_vars[e - 3] += (nd_var)
                #na_vars[e - 3] += (na_var)
                x[e - 3] += width
                count += 1

    x /= count
    l_vars /= count
    n_vars /= count
    nd_vars /= count
    #na_vars /= count
    plt.scatter(x, l_vars, label='learnability', alpha=0.7)
    plt.scatter(x, n_vars, label='absolute novelty', alpha=0.7)
    plt.scatter(x, nd_vars, label='novelty with decay', alpha=0.7)
    #plt.scatter(x, na_vars, label='novelty with average', alpha=0.7)

    plt.xlabel('artifact width')
    plt.xscale('log', base=2)

    plt.ylabel('measure variance')

    plt.title(f'{scenario} measure variance vs artifact width')

    plt.legend()

    plt.show()
