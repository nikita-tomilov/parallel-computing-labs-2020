#!/bin/bash

rm -rf logs-*

run() {
  M=$1
  for N in `echo 1000 5000 10000 $(seq 15000 15000 450000)`; do
    ./lab6 $N $M 200 | grep ':' | tee -a logs-$M.txt
  done
}

results_allsteps_total() {
  for M in logs-1.txt logs-2.txt logs-3.txt logs-4.txt logs-5.txt logs-6.txt logs-12.txt; do
    grep "Milliseconds" < "$M" > "$M"'a'
    sed -i s/'. Milliseconds passed: '/','/g *.txta
    sed -i s/'N=[0-9]*'/''/g *.txta
  done
  pr -t -m logs-1.txta logs-2.txta logs-3.txta logs-4.txta logs-5.txta logs-6.txta logs-12.txta | tee results-opencl-allsteps-total.csv
  rm -rf *.txta
}

results_step() {
  STEP=$1
  for M in logs-1.txt logs-2.txt logs-3.txt logs-4.txt logs-5.txt logs-6.txt logs-12.txt; do
    grep "$STEP" < "$M" > "$M"'a'
    sed -i s/'[a-zA-Z ]*:'/','/g *.txta
  done
  pr -t -s" " -w 120 -m logs-1.txta logs-2.txta logs-3.txta logs-4.txta logs-5.txta logs-6.txta logs-12.txta | tee 'results-opencl-allsteps-'"$STEP"'.csv'
  rm -rf *.txta
}

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
results_step 'Prof map'
results_step Merge
results_step 'Prof merge'
results_step Sort
results_step Reduce
rm -rf logs-*

mkdir -p "results_bigN_opencl"
mv results-*.csv ./results_bigN_opencl/