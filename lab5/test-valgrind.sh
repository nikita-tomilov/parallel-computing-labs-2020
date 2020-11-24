#!/bin/bash
rm -rf cachegrind*
run() {
  M=$1
  for N in $(seq 1200 3380 35000); do
     valgrind --tool=cachegrind ./lab5 $N $M 150 >/dev/null
  done
}

for M in $(seq 1 6); do
  echo "Threads count:" $M
  run $M
done

M=12
echo "Threads count:" $M
run $M
rm -rf cachegrind*
