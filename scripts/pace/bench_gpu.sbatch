#!/bin/bash
# Author: Shravya Vasudeva | ECE 6122 | Last Date Modified: 10/23/2026
# CPU vs GPU comparison, per-frame copy timing, and CPU/GPU validation.
# Submit from the repository root:  sbatch scripts/pace/bench_gpu.sbatch
#SBATCH -J swarm_gpu
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=12
#SBATCH --gres=gpu:1                # add a type if needed, e.g. --gres=gpu:V100:1
#SBATCH --mem=16G
#SBATCH -t 00:45:00
#SBATCH -o results/logs/%x_%j.out
set -euo pipefail
source scripts/pace/env.sh
mkdir -p results/logs

echo "node: $(hostname)"; nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv; nvcc --version | tail -1
lscpu | grep -E "Model name|^CPU\(s\)"

N=${SLURM_CPUS_PER_TASK:-12}
TAG="${SLURM_JOB_ID}"

# 1) CPU (all cores) vs CUDA across agent counts; copy is timed separately (copy_ms_* columns).
./build-pace/swarm_bench --backends cpu,cuda --agents 10k,50k,100k,250k,500k --threads "${N}" \
    --steps 200 --warmup 20 --repeats 3 --out "results/bench_gpu_${TAG}.csv"

# 2) Larger GPU-only sizes (skip metrics: they are O(samples x N) on the host).
./build-pace/swarm_bench --backends cuda --agents 1000k,2000k --steps 100 --no-metrics \
    --out "results/bench_gpu_${TAG}.csv" --append

# 3) Validation: same initial state on both backends, compare reference metrics (5% tolerance).
./build-pace/swarm_bench --validate --reference cpu --candidates cuda --agents 10k,50k,100k \
    --steps 300 --tolerance 0.05 --out "results/validation_${TAG}.csv" || echo "VALIDATION FAILED - see CSV"
