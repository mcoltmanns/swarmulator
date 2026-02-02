import h5py as h5
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import sys
from tqdm import tqdm

base_path = sys.argv[1]

for scenario in ['forage', 'pd', 'pred-prey']:
    for i in range(1, 6):
        ls_t = [] # avg learnabilities across width and step vs time
        ns_t = [] # same for novelties
        for e in range(3, 8):
            width = 2 ** e # artifact width is effectively the organizational scale: low width/high scale, high width/low scale
            print(scenario, i, width)
            if scenario == 'pred-prey':
                normal_measures = h5.File(f'{base_path}/{scenario}/pred_prey_{i}_observer_w{width}.h5', 'r')
                avg_measures = h5.File(f'{base_path}/{scenario}/pred_prey_{i}_observer_avg_w{width}.h5', 'r')
            else:
                normal_measures = h5.File(f'{base_path}/{scenario}/{scenario}_{i}_observer_w{width}.h5', 'r')
                avg_measures = h5.File(f'{base_path}/{scenario}/{scenario}_{i}_observer_avg_w{width}.h5', 'r')

            delta = normal_measures['chunk_size'][()]
            num = len(normal_measures['chunk_start'])
            x = (np.arange(0, num * delta, delta) + delta / 2) * 20

            ls = [] # avg learnabilities vs time at width (we already know our width because of where we are in the loop)
            ns = [] # avg novelties vs time at width

            for s in range(4, 8):
                step = 2 ** s
                l_s = normal_measures[f'learnability_step{step}'][()] # learnability vs time at step
                n_s = avg_measures[f'novelty_step{step}'][()]
                ls.append(l_s)
                ns.append(n_s)

            ls_t.append(np.mean(ls, axis=0))
            ns_t.append(np.mean(ns, axis=0))

        l_avg = np.mean(ls_t, axis=0)
        n_avg = np.mean(ns_t, axis=0)

        l_trend = np.polynomial.polynomial.polyfit(x, l_avg, deg=1)
        n_trend = np.polynomial.polynomial.polyfit(x, n_avg, deg=1)

        plt.plot(x, l_avg, label=f'learnability (avg.={round(np.mean(l_avg), 2)})', color='blue')
        plt.plot(x, l_trend[0] + x * l_trend[1], label=f'learnability trend {l_trend[1]:.1g}', color='blue', linestyle='--')
        plt.plot(x, n_avg, label=f'novelty (avg.={round(np.mean(n_avg), 2)})', color='red')
        plt.plot(x, n_trend[0] + x * n_trend[1], label=f'novelty trend {n_trend[1]:.1g}', color='red', linestyle='--')
        plt.legend()
        plt.ylim(0, max(max(l_avg), max(n_avg)))
        plt.xlim(0, max(x))
        plt.xlabel('simulation time')
        plt.ylabel('score')

        plt.savefig(f'{base_path}/{scenario}/{i}_trend.pdf', bbox_inches='tight')
        plt.close()
