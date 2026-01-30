#!/bin/bash

for i in {1..5}
do
    for e in {0..0}
    do
        python plotter.py /media/sda/forage/forage_"$i"_sigma_"$((10 ** e))".h5 "$((20 * 500000 / (500000 / (10 ** e))))" time sigma 75 0 "$((500000 / (10 ** e)))" /ForageAgent_avg_satisfaction /system_satisfaction

        python plotter.py /media/sda/pd/pd_"$i"_sigma_"$((10 ** e))".h5 "$((20 * 500000 / (500000 / (10 ** e))))" time sigma 75 0 "$((500000 / (10 ** e)))" /NeuralAgent_avg_satisfaction /system_satisfaction

        python plotter.py /media/sda/pred-prey/pred_prey_"$i"_sigma_"$((10 ** e))".h5 "$((20 * 500000 / (500000 / (10 ** e))))" time sigma 75 0 "$((500000 / (10 ** e)))" /Predator_avg_satisfaction /Prey_avg_satisfaction /Plant_avg_satisfaction /system_satisfaction
    done
done

