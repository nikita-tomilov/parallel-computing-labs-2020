#!/bin/bash
echo "On single thread"
./test-seq.sh
for T in `seq 2 2 8`; do
	echo "Threads count:" $T
	for N in `seq 2300 3470 37000`; do
		./lab1-par-$T $N | grep passed
	done
done
