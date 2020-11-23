#!/bin/bash

rm -rf logs-*

run() {
  M=$1
  S=$2
  for N in $(seq 1200 3380 35000); do
    ./lab5 $N $M $S | grep 'passed' | tee -a logs-$M.txt
  done
}

results_allsteps_total() {
  S=$1
  for M in logs-1.txt logs-2.txt logs-3.txt logs-4.txt logs-5.txt logs-6.txt; do
    grep "Milliseconds" < "$M" > "$M"'a'
    sed -i s/'. Milliseconds passed: '/','/g *.txta
    sed -i s/'N=[0-9]*'/''/g *.txta
  done
  pr -t -m logs-1.txta logs-2.txta logs-3.txta logs-4.txta logs-5.txta logs-6.txta | tee results-batchsize-$S.csv
  rm -rf *.txta
}

for S in 20 100 150 200 500; do
  for M in $(seq 1 6); do
    echo "Threads count:" $M "Batch size" $S
    run $M $S
  done
  results_allsteps_total $S
done

rm -rf logs-*
