#!/bin/bash
CC=/opt/intel/bin/icc
$CC -fast -Wall -Werror -o lab1-seq lab1.c -lm
#2 threads just to see if the parallel computing actually gives performance boost
$CC -fast -Wall -Werror -parallel -qopt-report-phase=par -par-threshold=2 lab1.c -o lab1-par-2 -lm
#4 threads because the pc has 6 physical cores
$CC -fast -Wall -Werror -parallel -qopt-report-phase=par -par-threshold=4 lab1.c -o lab1-par-4 -lm
#6 threads because the pc has 6 physical cores and to utilize all of them
$CC -fast -Wall -Werror -parallel -qopt-report-phase=par -par-threshold=6 lab1.c -o lab1-par-6 -lm
#8 threads because the pc has 6 physical cores and add two more threads
$CC -fast -Wall -Werror -parallel -qopt-report-phase=par -par-threshold=8 lab1.c -o lab1-par-8 -lm
#12 threads because the pc has 6 physical cores and two threads per core
$CC -fast -Wall -Werror -parallel -qopt-report-phase=par -par-threshold=12 lab1.c -o lab1-par-12 -lm
