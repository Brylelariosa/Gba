#!/bin/sh
# Builds and runs every host-side test with AddressSanitizer + UBSan.
set -e
cd "$(dirname "$0")"
SRC=$(ls ../src/*.c | grep -v '/main.c')
FLAGS="-std=gnu11 -O1 -g -fsanitize=address,undefined -fno-strict-aliasing -DHOST_TEST -Wall -Wextra -I../src"
echo "== gameplay tests =="
gcc $FLAGS $SRC game_test.c -o game_test && ./game_test | tail -3
echo "== scroll smoothness =="
gcc $FLAGS $SRC scroll_test.c -o scroll_test && ./scroll_test | tail -2
echo "== window offsets match re-centred renders =="
RSRC="../src/world.c ../src/render.c ../src/texgen.c ../src/palette.c ../src/hw.c ../src/blocks.c ../src/items.c ../src/rng.c ../src/move.c"
gcc $FLAGS $RSRC offset_test.c -o offset_test && ./offset_test
echo "== screenshots + long random bot run =="
gcc $FLAGS $SRC host_main.c -o host_test && mkdir -p out && ./host_test out | tail -2 && python3 render.py out > /dev/null && echo "PNGs in host/out"
