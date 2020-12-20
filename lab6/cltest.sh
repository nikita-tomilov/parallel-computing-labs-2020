#!/bin/bash
gcc test_opencl.c -lOpenCL
./a.out
rm -rf a.out
