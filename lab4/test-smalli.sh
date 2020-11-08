#!/bin/bash

./build-gcc-noopenmp.sh
rm -rf logs-*

compile() {
  CC=gcc-9
  TMP=lab4-tmp.c
  sed 's/#define CONST_NUM_THREADS [0-9]*$/#define CONST_NUM_THREADS '$1'/g' lab4.c > $TMP
  sed 's/#define MAX_I [0-9]*$/#define MAX_I 10/g' lab4.c > $TMP
  cat $TMP | grep '#define CONST_NUM_THREADS'
  cat $TMP | grep '#define MAX_I'
  $CC -O3 -Wall -Werror -fopenmp -o lab4 $TMP -lm
  rm -rf $TMP
}

run() {
  M=$1
  K=$2
  for N in $(seq 1200 3380 35000); do
    ./lab4 $N | grep passed | tee -a logs-$M-$K.txt
  done
}

results() {
  M=$1
  sed -i s/'. Milliseconds passed: '/','/g *.txt
  sed -i s/'N=[0-9]*'/''/g *.txt
  pr -t -m logs-*.txt | tee results-smalli-"$M".csv
  rm -rf logs-*
}

for M in $(seq 1 6); do
  echo "Threads count:" $M
  compile $M
  for K in $(seq 1 10); do
    echo "iteration" $K
    run $M $K
  done
  results $M
done
