#!/bin/bash
rm -rf logs-*
date
for M in `seq 1 6`; do
	echo "Threads count:" $M
	./lab2 35000 $M | grep passed
	sleep 2
done
echo "done"
date
