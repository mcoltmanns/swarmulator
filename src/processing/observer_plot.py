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

for scenario in ['forage', 'pd', 'pred-prey']:
    for i in range(1, 6):
        ls_w_s_t = [] # learnabilities vs width, step, time. 3d array. first index is width, second is step, third is time
        ns_w_s_t = [] # same for novelties
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

            ls = [] # learnabilities vs step (we already know our width because of where we are in the loop)
            ns = [] # novelties vs step

            colors = ['red', 'green', 'blue', 'orange']
            color_handles = []

            for s, c in zip(range(4, 8), colors):
                step = 2 ** s
                l = normal_measures[f'learnability_step{step}'][()]
                n = avg_measures[f'novelty_step{step}'][()]
                plt.plot(x, l, label=f'L step {step}', marker='o', alpha=0.5, color=c, linestyle='--')
                plt.plot(x, n, label=f'N step {step}', marker='s', alpha=0.5, color=c, linestyle='--')
                color_handles.append(Line2D([0], [0], color=c, linewidth=2, label=f'step {step}', linestyle='--'))
                ls.append(l)
                ns.append(n)

            ls_w_s_t.append(ls)
            ns_w_s_t.append(ns)

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
            plt.ylim(0, 1)
            plt.xlim(0, max(x))
            plt.savefig(f'{base_path}/{scenario}/{i}_{width}_observer.pdf')
            plt.close()

        fig = plt.figure(figsize=(16, 10))
        gs = fig.add_gridspec(4, 8, hspace=0.3, wspace=0.1)
        all_ax = []

        ns_w_s_t = np.array(ns_w_s_t)
        ls_w_s_t = np.array(ls_w_s_t)
        steps = [2 ** s for s in range(4, 8)]
        widths = [2 ** w for w in range(3, 8)]

        x_top = x[0:int(len(x) / 2)]
        x_bottom = x[int(len(x) / 2):]

        # novelty heatmap top
        for j, t in enumerate(x_top):
            axes = fig.add_subplot(gs[0, j])
            im_nov = axes.imshow(ns_w_s_t[:, :, j], aspect='auto', cmap='plasma', origin='lower', interpolation='nearest', vmin=0)
            axes.set_title(f't={np.round(t).astype(int)}', fontsize=10)

            if j == 0:
                axes.set_ylabel('Artifact width', fontsize=9)
                axes.set_yticks(range(len(widths)))
                axes.set_yticklabels(widths)
            else:
                axes.set_yticks([])
            axes.set_xticks(range(len(steps)))
            axes.set_xticklabels(steps, fontsize=8)
            all_ax.append(axes)

        # novelty heatmap bottom
        for j, t in enumerate(x_bottom):
            axes = fig.add_subplot(gs[1, j])
            axes.imshow(ns_w_s_t[:, :, j + len(x_top)], aspect='auto', cmap='plasma', origin='lower', interpolation='nearest', vmin=0)
            axes.set_title(f't={np.round(t).astype(int)}', fontsize=10)

            if j == 0:
                axes.set_ylabel('Artifact width', fontsize=9)
                axes.set_yticks(range(len(widths)))
                axes.set_yticklabels(widths)
            else:
                axes.set_yticks([])
            axes.set_xticks(range(len(steps)))
            axes.set_xticklabels(steps, fontsize=8)
            all_ax.append(axes)

        # learnability heatmap top
        for j, t in enumerate(x_top):
            axes = fig.add_subplot(gs[2, j])
            im_lrn = axes.imshow(ls_w_s_t[:, :, j], aspect='auto', cmap='viridis', origin='lower', interpolation='nearest', vmin=0)
            axes.set_title(f't={np.round(t).astype(int)}', fontsize=10)

            if j == 0:
                axes.set_ylabel('Artifact width', fontsize=9)
                axes.set_yticks(range(len(widths)))
                axes.set_yticklabels(widths)
            else:
                axes.set_yticks([])
            axes.set_xticks(range(len(steps)))
            axes.set_xticklabels(steps, fontsize=8)
            all_ax.append(axes)

        # learnability heatmap bottom
        for j, t in enumerate(x_bottom):
            axes = fig.add_subplot(gs[3, j])
            axes.imshow(ls_w_s_t[:, :, j + len(x_top)], aspect='auto', cmap='viridis', origin='lower', interpolation='nearest', vmin=0)
            axes.set_title(f't={np.round(t).astype(int)}', fontsize=10)

            if j == 0:
                axes.set_ylabel('Artifact width', fontsize=9)
                axes.set_yticks(range(len(widths)))
                axes.set_yticklabels(widths)
            else:
                axes.set_yticks([])

            axes.set_xlabel('Step', fontsize=9)
            axes.set_xticks(range(len(steps)))
            axes.set_xticklabels(steps, fontsize=8)
            all_ax.append(axes)

        fig.text(0.075, 0.75, 'Novelty', rotation=90, va='center', ha='center', fontsize=12, weight='bold')
        fig.text(0.075, 0.25, 'Learnability', rotation=90, va='center', ha='center', fontsize=12, weight='bold')

        plt.tight_layout(rect=[0.03, 0, 1, 1])

        fig.colorbar(im_nov, ax=all_ax[0:15], location='right', shrink=0.8, label='Score', pad=0.02)
        fig.colorbar(im_lrn, ax=all_ax[15:], location='right', shrink=0.8, label='Score', pad=0.02)

        plt.savefig(f'{base_path}/{scenario}/{i}_heatmaps.pdf', bbox_inches='tight')
        plt.close()
