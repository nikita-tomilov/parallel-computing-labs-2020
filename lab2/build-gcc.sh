#!/bin/bash
CC=gcc-9
$CC -O3 -Wall -Werror -o lab2 lab2.c -L. -lm -lfwSignal -lfwBase