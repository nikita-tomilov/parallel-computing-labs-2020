#!/bin/bash
#2 threads just to see if the parallel computing actually gives performance boost
gcc -O3 -Wall -Werror -floop-parallelize-all -ftree-parallelize-loops=2 lab1.c -o lab1-par-2 -lm
#4 threads because the laptop has 4 physical cores
gcc -O3 -Wall -Werror -floop-parallelize-all -ftree-parallelize-loops=4 lab1.c -o lab1-par-4 -lm
#6 threads because the laptop has 4 physical cores and to utilize two more logical cores
gcc -O3 -Wall -Werror -floop-parallelize-all -ftree-parallelize-loops=6 lab1.c -o lab1-par-6 -lm
#8 threads because the laptop has 8 logical cores
gcc -O3 -Wall -Werror -floop-parallelize-all -ftree-parallelize-loops=8 lab1.c -o lab1-par-8 -lm
