#!/bin/bash
CC=gcc-9
$CC -O3 -Wall -Werror -fopenmp -o lab3 lab3.c -lm
