#!/bin/bash

# do one run with sample on all artifact widths - check if different widths give different results (maybe they capture more detail?)
# run out of memory for sampen at widths >= 32
#for a in {3..7}
#do
#    echo PD run 1 width "$((2 ** a))"
#    python multiscale_entropy.py /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_1_arts_"$((2 ** a))".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_1_entropy.h5 width"$((2 ** a))" MvSampEn
#    python multiscale_entropy.py /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_1_arts_"$((2 ** a))".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_1_entropy.h5 width"$((2 ** a))" MvPermEn
#done

for a in {4..4}
do
    for i in {1..5}
    do
        #echo Forage run "$i", width "$((2 ** a))"
        #python multiscale_entropy.py /mnt/15799271-aa42-4864-92d2-d1db24402001/forage/forage_"$i"_arts_"$((2 ** a))".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/forage/forage_"$i"_entropy.h5 width"$((2 ** a))" MvSampEn

        echo PD run "$i", width "$((2 ** a))"
        python multiscale_entropy.py /media/sda/pd/pd_"$i"_arts_"$((2 ** a))".h5 /media/sda/pd/pd_"$i"_entropy.h5 width"$((2 ** a))" MvSampEn

        echo pred prey run "$i", width "$((2 ** a))"
        python multiscale_entropy.py /media/sda/pred-prey/pred_prey_"$i"_arts_"$((2 ** a))".h5 /media/sda/pred-prey/pred_prey_"$i"_entropy.h5 width"$((2 ** a))" MvSampEn
    done
done
