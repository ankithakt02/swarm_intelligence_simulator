#!/bin/bash
# Author: Shravya Vasudeva | ECE 6122 | Last Date Modified: 10/23/2026
# Builds the headless benchmark on PACE-ICE (no SFML window on batch nodes).
# Run from the repository root:  bash scripts/pace/build.sh
# Build ON A GPU NODE (or with nvcc loaded) so the CUDA backend is compiled in.
set -euo pipefail
source "$(dirname "$0")/env.sh"

# V100 = 70, RTX 6000 = 75, A100 = 80, H100/H200 = 90. Default builds all of 70;75;80;90.
ARCHS="${CUDA_ARCHS:-70;75;80;90}"

cmake -S . -B build-pace -DCMAKE_BUILD_TYPE=Release -DSWARM_BUILD_GUI=OFF \
      -DCMAKE_CUDA_ARCHITECTURES="${ARCHS}"
cmake --build build-pace -j "${SLURM_CPUS_ON_NODE:-8}"
./build-pace/swarm_bench --list-backends
