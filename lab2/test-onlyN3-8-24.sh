#!/bin/bash
rm -rf logs-*
date
for M in `echo 8 10 12 16 20 24`; do
	echo "Threads count:" $M
	./lab2 1000000 $M | grep passed
	sleep 2
done
echo "done"
date
