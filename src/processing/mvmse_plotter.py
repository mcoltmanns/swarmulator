import h5py as h5
import numpy as np
import matplotlib.pyplot as plt
import sys

in_file = h5.File(sys.argv[1], 'r')

# first plot sample entropy
for e in range(3, 5):
    width = 2 ** e
    sampen = in_file[f'width{width}']['MvSampEn entropy'][()]
    ci = in_file[f'width{width}']['MvSampEn complexity index'][()]
    plt.plot([x for x in range(0, len(sampen))], sampen, label=f'SampEn (w={width}, ci={round(ci, 2)})')

plt.legend()
plt.xlim(0, len(sampen) - 1)
plt.xlabel('scale')
plt.xticks(range(0, len(sampen) - 1, 2))
plt.ylabel('sample entropy')
plt.savefig(f'{sys.argv[1]}_sampenplot.pdf', bbox_inches='tight')

plt.close()

# then perm entropy
for e in range(3, 8):
    width = 2 ** e
    permen = in_file[f'width{width}']['entropy'][()]
    ci = in_file[f'width{width}']['complexity index'][()]
    plt.plot([x for x in range(0, len(permen))], permen, label=f'PermEn (w={width}, ci={round(ci, 2)})')

plt.legend()
plt.xlim(0, len(permen) - 1)
plt.xlabel('scale')
plt.xticks(range(0, len(permen) - 1, 2))
plt.ylabel('permutation entropy')
plt.savefig(f'{sys.argv[1]}_permenplot.pdf', bbox_inches='tight')

in_file.close()
