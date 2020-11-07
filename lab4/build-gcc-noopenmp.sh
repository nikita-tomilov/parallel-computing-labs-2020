#!/bin/bash
CC=gcc-9
$CC -O3 -Wall -o lab4nomp lab4.c -lm -lpthread
