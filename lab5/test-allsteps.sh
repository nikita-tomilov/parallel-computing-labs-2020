#!/bin/bash

rm -rf logs-*

run() {
  M=$1
  for N in $(seq 1200 3380 35000); do
    ./lab5 $N $M 150 | grep ':' | tee -a logs-$M.txt
  done
}

run_nomp() {
  for N in $(seq 1200 3380 35000); do
    ./lab5 $N 0 | grep ':' | tee -a logs-nopthread.txt
  done
}

results_allsteps_total() {
  for M in logs-nopthread.txt logs-1.txt logs-2.txt logs-3.txt logs-4.txt logs-5.txt logs-6.txt logs-12.txt; do
    grep "Milliseconds" < "$M" > "$M"'a'
    sed -i s/'. Milliseconds passed: '/','/g *.txta
    sed -i s/'N=[0-9]*'/''/g *.txta
  done
  pr -t -m logs-nopthread.txta logs-1.txta logs-2.txta logs-3.txta logs-4.txta logs-5.txta logs-6.txta logs-12.txta | tee results-allsteps-total.csv
  rm -rf *.txta
}

results_step() {
  STEP=$1
  for M in logs-nopthread.txt logs-1.txt logs-2.txt logs-3.txt logs-4.txt logs-5.txt logs-6.txt logs-12.txt; do
    grep "$STEP" < "$M" > "$M"'a'
    sed -i s/'[a-zA-Z]*:'/','/g *.txta
  done
  pr -t -s" " -w 120 -m logs-nopthread.txta logs-1.txta logs-2.txta logs-3.txta logs-4.txta logs-5.txta logs-6.txta logs-12.txta | tee 'results-allsteps-'"$STEP"'.csv'
  rm -rf *.txta
}

echo "No OpenMP"

run_nomp

for M in $(seq 1 6); do
  echo "Threads count:" $M
  run $M
done

M=12
echo "Threads count:" $M
run $M

results_allsteps_total
results_step Generate
results_step Map
results_step Merge
results_step Sort
results_step Reduce
rm -rf logs-*
