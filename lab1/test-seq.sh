#!/bin/bash
for N in `seq 1200 3380 35000`; do
	./lab1-seq $N | grep passed
done
