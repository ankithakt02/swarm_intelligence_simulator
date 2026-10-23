# Measurable results and targets (requested by the TA)

All numbers come from PACE-ICE jobs in `scripts/pace/`; `scripts/plot_results.py` turns the CSVs into the
tables and figures. Targets are our predictions before measuring; the report states whether each was met.

| # | Result | How it is measured | Target | Where |
|---|---|---|---|---|
| 1 | CPU step time and speedup, 1 to N threads | `bench_cpu.sbatch`: 10k / 50k / 100k agents, threads 1,2,4,8,12,16,N, 3 repeats | >= 4x at 8 threads, >= 8x at 24 threads (50k agents) | `thread_scaling.png`, summary table |
| 2 | CPU vs GPU step time | `bench_gpu.sbatch`: 10k / 50k / 100k / 250k / 500k agents, CPU at all cores | GPU >= 10x faster than CPU at 100k agents | `step_time_vs_agents.png` |
| 3 | Largest agent count simulated (no rendering) within a 60 FPS budget (16.7 ms/step) | same run, plus 1M / 2M GPU-only | CPU >= 100k, GPU >= 1M | `step_time_vs_agents.png` |
| 4 | Largest agent count **rendered** at 30 and 60 FPS | `swarm --fps-sweep 10k,25k,50k,100k,200k,400k --sweep-backends cpu,cuda` on the demo machine | GPU: 100k at 60 FPS; 200k+ at 30 FPS | `frame_breakdown_*.png`, summary table |
| 5 | Per-frame GPU -> host copy | `copy_ms_*` columns (benchmark) and `copy_pct_of_frame` (FPS sweep), timed separately from the step | copy < 25% of frame time at 100k agents | summary tables |
| 6 | CPU/GPU agreement | `swarm_bench --validate --reference cpu --candidates cuda`, same seed, 300 steps | alignment, cohesion, nearest-neighbour distance, mean speed within 5% relative (alignment also passes within 0.02 absolute) | validation table |

Notes for the report:

- Compare **statistics, not trajectories**: floating-point summation order differs between backends, so individual agents diverge after a few hundred steps even when both are correct.
- Report the node (CPU model, GPU model, `nvidia-smi` output) for every table; the job logs in `results/logs/` have it.
- If a target is missed, say why (e.g. copy bandwidth, grid build cost, memory-bound kernel) and what was tried.
