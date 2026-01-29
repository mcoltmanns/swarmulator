#!/bin/bash

for i in {1..5}
do
    python plotter.py /run/media/moltmanns/4fd63045-5e13-4cda-bc6b-1abb8f299c37/forage/forage_"$i".h5 20 time "avg. signal strength" 200 0 500000 /objects/ForageAgent/sig_a /objects/ForageAgent/sig_b

    python plotter.py /run/media/moltmanns/4fd63045-5e13-4cda-bc6b-1abb8f299c37/pd/pd_"$i".h5 20 time "avg. signal strength" 200 0 500000 /objects/NeuralAgent/sig_a /objects/NeuralAgent/sig_b

    python plotter.py /run/media/moltmanns/4fd63045-5e13-4cda-bc6b-1abb8f299c37/pred-prey/pred_prey_"$i".h5 20 time "avg. signal strength" 200 0 500000 /objects/Predator/sig_a /objects/Predator/sig_b /objects/Prey/sig_a /objects/Prey/sig_b

done

