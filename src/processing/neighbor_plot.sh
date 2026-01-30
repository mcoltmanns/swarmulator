#!/bin/bash

for i in {1..5}
do
    python plotter.py /run/media/moltmanns/4fd63045-5e13-4cda-bc6b-1abb8f299c37/forage/forage_"$i".h5 20 time "avg. neighbor count" 200 0 500000 /objects/ForageAgent/avg_neighbor_count

    python plotter.py /run/media/moltmanns/4fd63045-5e13-4cda-bc6b-1abb8f299c37/pd/pd_"$i".h5 20 time "avg. neighbor count" 200 0 500000 /objects/NeuralAgent/avg_neighbor_count

    python plotter.py /run/media/moltmanns/4fd63045-5e13-4cda-bc6b-1abb8f299c37/pred-prey/pred_prey_"$i".h5 20 time "avg. neighbor count" 200 0 500000 /objects/Predator/avg_neighbor_count /objects/Prey/avg_neighbor_count
done

