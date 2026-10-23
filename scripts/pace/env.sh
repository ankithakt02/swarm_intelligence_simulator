#!/bin/bash
# Author: Shravya Vasudeva | ECE 6122 | Last Date Modified: 10/23/2026
# Loads the toolchain on PACE-ICE. Source it:  source scripts/pace/env.sh
# Module names/versions change between semesters - check with:  module avail gcc cuda cmake
module purge
module load gcc        # C++17 compiler
module load cmake      # >= 3.18
module load cuda       # nvcc; only needed for the CUDA backend
module list 2>&1
