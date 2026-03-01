#!/bin/bash

for i in {1..5}
do
    for e in {0..0}
    do
        #python plotter.py /media/sda/forage/forage_"$i"_sigma_"$((10 ** e))".h5 "$((20 * 500000 / (500000 / (10 ** e))))" time friction 75 0 "$((500000 / (10 ** e)))" /ForageAgent_internal_friction /system-ForageAgent_friction

        #python plotter.py /media/sda/pd/pd_"$i"_sigma_"$((10 ** e))".h5 "$((20 * 500000 / (500000 / (10 ** e))))" time friction 75 0 "$((500000 / (10 ** e)))" /NeuralAgent_internal_friction /system-NeuralAgent_friction

        python plotter.py /media/sda/pred-prey/pred_prey_"$i"_sigma_"$((10 ** e))".h5 "$((20 * 500000 / (500000 / (10 ** e))))" time friction 75 0 "$((500000 / (10 ** e)))" /Predator-Prey_friction /Predator_internal_friction /Prey_internal_friction
    done
done

