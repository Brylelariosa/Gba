#!/bin/sh
# usage: host/run.sh host/t2.txt /tmp/shot     (run from the repo root; needs gcc, python3, numpy, Pillow)
set -e
mkdir -p build_host
gcc -DHOST_BUILD -std=gnu11 -O1 -g -fsanitize=address,undefined src/*.c host/host_main.c -o build_host/sim
./build_host/sim "$1" "$2"
python3 tools/ppu.py "$2"_*.bin
