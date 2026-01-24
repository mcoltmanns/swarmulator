#!/bin/bash

# unlike the other measures, the base skip passed is increased in the python code (start at n, go to 20 * n with a step of n)
# here we start at a skip of 10 (200 real timesteps) and go to 200
# params are input file, output file, lookback (after step), number of places to calculate at, min step, max step

for a in {3..7}
do
    for i in {1..5}
    do
        echo Forage run "$i", width "$((2 ** a))"
        python observer_runner.py /mnt/15799271-aa42-4864-92d2-d1db24402001/forage/forage_"$i"_arts_"$((2 ** a))".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/forage/forage_"$i"_observer_decay_w"$((2 ** a))".h5 128 15 16 512

        echo PD run "$i", width "$((2 ** a))"
        python observer_runner.py /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_"$i"_arts_"$((2 ** a))".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_"$i"_observer_decay_w"$((2 ** a))".h5 128 15 16 512

        echo Pred prey run "$i", width "$((2 ** a))"
        python observer_runner.py /mnt/15799271-aa42-4864-92d2-d1db24402001/pred-prey/pred_prey_"$i"_arts_"$((2 ** a))".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/pred-prey/pred_prey_"$i"_observer_decay_w"$((2 ** a))".h5 128 15 16 512

    done
done
