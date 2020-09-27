#!/bin/bash
rm -rf logs-*
for M in `seq 1 6`; do
	echo "Threads count:" $M
	for N in `seq 1200 3380 35000`; do
		./lab2 $N $M | grep passed | tee -a logs-$M.txt
	done
done
sed -i s/'. Milliseconds passed: '/','/g *.txt
sed -i s/'N=[0-9]*'/''/g *.txt
pr -t -m logs-1.txt logs-2.txt logs-3.txt logs-4.txt logs-5.txt logs-6.txt | tee results.csv
rm -rf logs-*
