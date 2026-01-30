#!/bin/bash

for a in {0..0}
do
    echo Step "$((10 ** a))"
    for i in {1..5}
    do
        echo Forage run "$i"
        python sigma_profile.py /media/sda/forage/forage_"$i".h5 /media/sda/forage/forage_"$i"_sigma_"$((10 ** a))".h5 "$((10 ** a))" 500 SimObject

        echo PD run "$i"
        python sigma_profile.py /media/sda/pd/pd_"$i".h5 /media/sda/pd/pd_"$i"_sigma_"$((10 ** a))".h5 "$((10 ** a))" 400 SimObject
        
        echo Pred prey run "$i"
        python sigma_profile.py /media/sda/pred-prey/pred_prey_"$i".h5 /media/sda/pred-prey/pred_prey_"$i"_sigma_"$((10 ** a))".h5 "$((10 ** a))" 250 SimObject
    done
done

