rm -rf logs-*

#gcc lab6.c -o lab6 -lm -lpthread -lOpenCL

run() {
  M=$1
  for N in $(seq 1200 3380 35000); do
    for K in $(seq 1 10); do
      ./lab6 $N $M 200 | grep ':' | tee -a logs-$M-$K.txt
      sleep 1
    done
  done
}

results_allsteps_total() {
  for M in $(echo 1 2 3 4 5 6 12); do
    for K in $(seq 1 10); do
      FILENAME=logs-$M-$K.txt
      grep "Milliseconds" < "$FILENAME" > "$FILENAME"'a'
      sed -i s/'. Milliseconds passed: '/','/g *.txta
      sed -i s/'N=[0-9]*'/''/g *.txta
    done
    pr -W 256 -t -m *.txta | tee results-opencl-allsteps-threads-$M.csv
    rm -rf *.txta
  done
}

results_step() {
  STEP=$1

  for M in $(echo 1 2 3 4 5 6 12); do
    for K in $(seq 1 10); do
      FILENAME=logs-$M-$K.txt
      grep "$STEP" < "$FILENAME" > "$FILENAME"'a'
      sed -i s/'[a-zA-Z ]*:'/','/g *.txta
    done
    pr -W 256 -t -m *.txta | tee results-opencl-step-"$STEP"-threads-$M.csv
    rm -rf *.txta
  done
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

mkdir -p "results_opencl"
mv results-*.csv ./results_opencl/
