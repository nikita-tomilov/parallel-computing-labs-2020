#!/bin/bash

compile() {
  CC=gcc-9
  TMP=lab3-tmp.c
  sed 's/#define CONST_NUM_THREADS [0-9]*$/#define CONST_NUM_THREADS '$1'/g' lab3.c > $TMP.2
  sed 's/#define SCHEDULE_STRING.*$/#define SCHEDULE_STRING schedule('$2','$3')/g' $TMP.2 > $TMP
  cat $TMP | grep '#define CONST_NUM_THREADS'
  cat $TMP | grep '#define SCHEDULE_STRING'
  $CC -O3 -Wall -Werror -fopenmp -o lab3 $TMP -lm || die CE
  rm -rf $TMP $TMP.2
}

run() {
  M=$1
  for N in `seq 1200 3380 35000`; do
    ./lab3 $N | grep passed | tee -a logs-$M.txt
  done
}

result() {
  sed -i s/'. Milliseconds passed: '/','/g *.txt
  sed -i s/'N=[0-9]*'/''/g *.txt
  pr -t -m logs-1.txt logs-2.txt logs-3.txt logs-4.txt logs-5.txt logs-6.txt logs-12.txt | tee results-scheduling-$1-$2.csv
  rm -rf logs-*
}

run_assuming_sched() {
  SCHED=$1
  CHUNK=$2

  echo SCHED $SCHED CHUNK $CHUNK

  for M in `seq 1 6`; do
    echo "Threads count:" $M
    compile $M $SCHED $CHUNK
    run $M
  done

  M=12
  echo "Threads count:" $M
  compile $M $SCHED $CHUNK
  run $M

  result $SCHED $CHUNK
}

start=`date +%s`

for SCHED in `echo static dynamic guided`; do
  for CHUNK in `echo 2 20 200 2000 5000 10000`; do
    run_assuming_sched $SCHED $CHUNK
  done
done

end=`date +%s`

runtime=$((end-start))
echo "WAS RUNNING " $runtime " seconds"
