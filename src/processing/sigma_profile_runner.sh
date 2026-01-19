#!/bin/bash

for a in {0..4}
do
    echo Step "$((10 ** a))"
    for i in {1..5}
    do
        echo Forage run "$i"
        python sigma_profile.py /mnt/15799271-aa42-4864-92d2-d1db24402001/forage/forage_"$i".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/forage/forage_"$i"_sigma_"$((10 ** a))".h5 "$((10 ** a))" SimObject

        echo PD run "$i"
        python sigma_profile.py /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_"$i".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_"$i"_sigma_"$((10 ** a))".h5 "$((10 ** a))" SimObject
        
        echo Pred prey run "$i"
        python sigma_profile.py /mnt/15799271-aa42-4864-92d2-d1db24402001/pred-prey/pred_prey_"$i".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/pred-prey/pred_prey_"$i"_sigma_"$((10 ** a))".h5 "$((10 ** a))" SimObject
    done
done

