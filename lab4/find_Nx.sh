#!/bin/bash
./build-gcc.sh
./build-gcc-noopenmp.sh
rm -rf logs-*
FROM=100
TO=7000
STEP=100
echo "No OpenMP"
for N in $(seq $FROM $STEP $TO); do
  ./lab4nomp "$N" | grep passed | tee -a logs-omp.txt
done
echo "With OpenMP"
for N in $(seq $FROM $STEP $TO); do
  ./lab4omp "$N" | grep passed | tee -a logs-nomp.txt
done
sed -i s/'. Milliseconds passed: '/','/g *.txt
sed -i s/'N=[0-9]*'/''/g *.txt
pr -t -m logs-omp.txt logs-nomp.txt | tee results-findNx.csv
rm -rf logs-*
