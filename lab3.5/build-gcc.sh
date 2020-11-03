#!/bin/bash
CC=gcc-9
$CC -O3 -Wall -Werror -fopenmp -o lab35omp lab3.5.c -lm
