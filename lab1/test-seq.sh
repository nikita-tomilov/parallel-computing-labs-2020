#!/bin/bash
for N in `seq 2300 3470 37000`; do
	./lab1-seq $N | grep passed
done
