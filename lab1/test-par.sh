#!/bin/bash
rm -rf *.txt
echo "On single thread"
./test-seq.sh | tee logs-1.txt
for T in `echo 2 4 6 8 12`; do
	echo "Threads count:" $T
	for N in `seq 2300 3470 37000`; do
		./lab1-par-$T $N | grep passed | tee -a logs-$T.txt
	done
done
sed -i s/'. Milliseconds passed: '/','/g *.txt
sed -i s/'N='/','/g *.txt
pr -t -m logs-1.txt logs-2.txt logs-4.txt logs-6.txt logs-8.txt logs-12.txt | tee results.csv
rm -rf *.txt
