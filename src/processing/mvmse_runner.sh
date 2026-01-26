#!/bin/bash

# do one run with sample on all artifact widths - check if different widths give different results (maybe they capture more detail?)
for a in {3..7}
do
    python multiscale_entropy.py /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_1_arts_"$((2 ** a))".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_1_entropy.h5 width"$((2 ** a))" MvSampEn
done

#for a in {3..7}
#do
#    for i in {1..5}
#    do
#        echo Forage run "$i", width "$((2 ** a))"
#        python multiscale_entropy.py /mnt/15799271-aa42-4864-92d2-d1db24402001/forage/forage_"$i"_arts_"$((2 ** a))".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/forage/forage_"$i"_entropy.h5 width"$((2 ** a))"
#
#        echo PD run "$i", width "$((2 ** a))"
#        python multiscale_entropy.py /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_"$i"_arts_"$((2 ** a))".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/pd/pd_"$i"_entropy.h5 width"$((2 ** a))"
#
#        echo pred prey run "$i", width "$((2 ** a))"
#        python multiscale_entropy.py /mnt/15799271-aa42-4864-92d2-d1db24402001/pred-prey/pred_prey_"$i"_arts_"$((2 ** a))".h5 /mnt/15799271-aa42-4864-92d2-d1db24402001/pred-prey/pred_prey_"$i"_entropy.h5 width"$((2 ** a))"
#    done
#done
