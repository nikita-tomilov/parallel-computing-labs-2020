#!/bin/bash

./build-gcc-noopenmp.sh
rm -rf logs-*

compile() {
  CC=gcc-9
  TMP=lab4-tmp.c
  sed 's/#define CONST_NUM_THREADS [0-9]*$/#define CONST_NUM_THREADS '$1'/g' lab4.c > $TMP
  cat $TMP | grep '#define CONST_NUM_THREADS'
  $CC -O3 -Wall -Werror -fopenmp -o lab4 $TMP -lm
  rm -rf $TMP
}

run() {
  M=$1
  for N in $(seq 1200 3380 35000); do
    ./lab4 $N | grep passed | tee -a logs-$M.txt
  done
}

run_nomp() {
  for N in $(seq 1200 3380 35000); do
    ./lab4nomp $N | grep passed | tee -a logs-nomp.txt
  done
}

results() {
  sed -i s/'. Milliseconds passed: '/','/g *.txt
  sed -i s/'N=[0-9]*'/''/g *.txt
  pr -t -m logs-nomp.txt logs-1.txt logs-2.txt logs-3.txt logs-4.txt logs-5.txt logs-6.txt logs-12.txt | tee results-default.csv
  rm -rf logs-*
}

echo "No OpenMP"

run_nomp

for M in $(seq 1 6); do
  echo "Threads count:" $M
  compile $M
  run $M
done

M=12
echo "Threads count:" $M
compile $M
run $M

results
