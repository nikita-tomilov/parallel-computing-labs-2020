#!/bin/bash
CC=gcc-9
$CC -O3 -Wall -Werror -fopenmp -o lab4omp lab4.c -lm
