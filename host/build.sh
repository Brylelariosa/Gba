#!/bin/sh
# Host test build with sanitizers (mimics the strict warnings of newer GCC).
set -e
SRC=$(ls ../src/*.c | grep -v '/main.c')
gcc -std=gnu11 -O1 -g -fsanitize=address,undefined -fno-strict-aliasing -DHOST_TEST \
    -Wall -Wextra -Werror=implicit-function-declaration -Werror=incompatible-pointer-types \
    -Werror=int-conversion -I../src $SRC host_main.c -o host_test
