#!/bin/sh
set -e
SRC=$(ls ../src/*.c | grep -v '/main.c')
gcc -std=gnu11 -O1 -g -fsanitize=address,undefined -fno-strict-aliasing -DHOST_TEST \
    -Wall -Wextra -I../src $SRC game_test.c -o game_test
