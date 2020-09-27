#!/bin/bash
rm -rf logs-*
for M in `echo 8 10 12 16 20 24`; do
	echo "Threads count:" $M
	for N in `seq 1200 3380 35000`; do
		./lab2 $N $M | grep passed | tee -a logs-$M.txt
	done
done
sed -i s/'. Milliseconds passed: '/','/g *.txt
sed -i s/'N=[0-9]*'/''/g *.txt
pr -t -m logs-8.txt logs-10.txt logs-12.txt logs-16.txt logs-20.txt logs-24.txt | tee results-2.csv
rm -rf logs-*
