import h5py as h5
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import sys
from tqdm import tqdm

"""
do one plot for each run/artifact width
plot solid lines for novelty, dashed lines for learnability
each step
"""

base_path = sys.argv[1]

normal_measures = h5.File(f'{base_path}/observer_test.h5', 'r')

delta = normal_measures['chunk_size'][()]
num = len(normal_measures['chunk_start'])
x = (np.arange(0, num * delta, delta) + delta / 2) * 20

ls = [] # learnabilities vs step (we already know our width because of where we are in the loop)
ns = [] # novelties vs step

colors = ['red', 'green', 'blue', 'orange']
color_handles = []

for s, c in zip(range(4, 8), colors):
    step = 2 ** s
    l = normal_measures[f'learnability_step{step}'][()]
    n = normal_measures[f'novelty_step{step}'][()]
    plt.plot(x, l, label=f'L step {step}', marker='o', alpha=0.5, color=c, linestyle='--')
    plt.plot(x, n, label=f'N step {step}', marker='s', alpha=0.5, color=c, linestyle='--')
    color_handles.append(Line2D([0], [0], color=c, linewidth=2, label=f'step {step}', linestyle='--'))
    ls.append(l)
    ns.append(n)

l_avg = np.mean(ls, axis=0)
n_avg = np.mean(ns, axis=0)
plt.plot(x, l_avg, label='L average', marker='o', alpha=0.7, color='black')
plt.plot(x, n_avg, label='N average', marker='s', alpha=0.7, color='black')
color_handles.append(Line2D([0], [0], color='black', linewidth=2, label='average'))

marker_handles = [
    Line2D([0], [0], color='gray', marker='o', linestyle='', markersize=6, label='learnability'),
    Line2D([0], [0], color='gray', marker='s', linestyle='', markersize=6, label='novelty')
]

plt.legend(handles=color_handles + marker_handles, ncol=3)
plt.ylim(0, max(np.max(ls), np.max(ns)) + 0.1 * max(np.max(ls), np.max(ns)))
plt.xlim(0, max(x))
plt.savefig(f'{base_path}/observer_test.png', bbox_inches='tight')
print('l avg', np.mean(l_avg), '+-', np.var(l_avg))
print('n avg', np.mean(n_avg), '+-', np.var(n_avg))
plt.close()
