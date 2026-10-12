#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
# Zig 0.13 provides the ARM64 compiler, glibc sysroot and static C++ library.
# No Docker, Python or compiler is needed on the handheld.
: "${ZIG:=zig}"
mkdir -p build-arm64
export ZIG_GLOBAL_CACHE_DIR="${ZIG_GLOBAL_CACHE_DIR:-$PWD/build-arm64/zig-global-cache}"
export ZIG_LOCAL_CACHE_DIR="${ZIG_LOCAL_CACHE_DIR:-$PWD/build-arm64/zig-local-cache}"
"$ZIG" c++ -target aarch64-linux-gnu.2.17 -mcpu=cortex_a53 \
    -std=c++17 -O2 -s -Wall -Wextra -Wpedantic -Iinclude -Ivendor/SDL2 \
    src/world.cpp src/generation.cpp src/ecology.cpp src/animals.cpp src/buildings.cpp src/population.cpp src/brain.cpp src/learning.cpp src/occupations.cpp src/society.cpp src/saves.cpp src/slots.cpp src/main.cpp src/port_art.cpp src/performance.cpp src/sound.cpp src/feedback.cpp src/sdl.cpp src/input.cpp src/rumble.cpp src/render.cpp src/sprites.cpp src/ui.cpp \
    -ldl -pthread -o build-arm64/living-village
echo "Built build-arm64/living-village (AArch64 Linux, glibc 2.17 baseline)"
