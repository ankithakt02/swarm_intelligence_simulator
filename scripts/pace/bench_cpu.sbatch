#!/bin/bash
# Author: Shravya Vasudeva | ECE 6122 | Last Date Modified: 10/23/2026
# CPU thread-scaling benchmark (report item: step time and speedup from 1 to N threads).
# Submit from the repository root:  sbatch scripts/pace/bench_cpu.sbatch
#SBATCH -J swarm_cpu
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=24          # whole node's cores so threads are not oversubscribed
#SBATCH --mem=16G
#SBATCH -t 00:45:00
#SBATCH -o results/logs/%x_%j.out
set -euo pipefail
source scripts/pace/env.sh
mkdir -p results/logs

echo "node: $(hostname)"; lscpu | grep -E "Model name|^CPU\(s\)|Thread|Socket"

N=${SLURM_CPUS_PER_TASK:-24}
THREADS="1,2,4,8,12,16,${N}"

# Thread scaling at three sizes, 3 repeats each (the plot script averages repeats).
./build-pace/swarm_bench --backends cpu --agents 10k,50k,100k --threads "${THREADS}" \
    --steps 200 --warmup 20 --repeats 3 --out "results/bench_cpu_${SLURM_JOB_ID}.csv"
