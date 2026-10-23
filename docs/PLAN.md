# Project plan: tiers, timeline, and dependencies

Submission deadline: end of November 2026. Internal target: **submit Sun Nov 29**, keep Nov 30 as buffer.

## Who owns what (by the TA's tiers)

| Tier | Ankitha (CPU backend) | Disha (CUDA backend) | Shravya (app, UI, benchmarks, build) |
|---|---|---|---|
| **0 - Foundation** (everyone) | Writes `src/sim/SimTypes.h` after the team agrees on it; defines the rule formulas | Reviews the interface for GPU needs (SoA, render-data download) | Repo, CMake, stub backend, app skeleton, benchmark CSV, PACE-ICE scripts *(done)* |
| **1 - Flocking on both backends + benchmarks** | CPU flocking, `SpatialGrid`, `ThreadPool`, reference metrics, thread-scaling runs | CUDA flocking, GPU grid (sort by cell), cheap `downloadRenderData`, kernel timings | Renderer + HUD on the real backends, benchmark sweeps, **separate per-frame copy timing**, first PACE runs |
| **1 - Validation** | Provides reference metrics + tolerance | Compares CUDA to CPU (`swarm_bench --validate`) | Validation CSV into the report tables |
| **2 - Foraging + predators** | Pheromone field, ant foraging, predator/prey on CPU | Same rules as kernels, `downloadPheromone` | Food/obstacle input, predator/ant colours, pheromone overlay |
| **3 - Live switching + full UI** | `uploadState`/`downloadState` on CPU | `uploadState`/`downloadState` on GPU | `SimulationController` switching *(done)*, sliders, buttons, panel |
| **Every tier** | Code comments, results for your section | Code comments, results for your section | Report targets, readme (SFML + CUDA versions with links), video, style pass, `Final_Project.zip` |

## Timeline

| Dates | Milestone | Ankitha | Disha | Shravya |
|---|---|---|---|---|
| **Oct 26 - 28** | **M0: interface frozen.** Everyone builds the repo locally and on PACE-ICE | Draft `SimTypes.h` changes + `Rules.h` (formulas) | Build CUDA path on PACE GPU node; `DeviceBuffers` + error macro | Repo set up, help teammates build, confirm the demo machine |
| **Oct 29 - Nov 1** | **M1: something real moves** | Single-thread CPU flocking with `SpatialGrid` | First flocking kernel (can be brute force), upload/download | Draw the CPU backend; HUD; test PACE scripts on real nodes |
| **Nov 2 - 10** | **M2: Tier 1 done** | `ThreadPool`, row partitioning, double buffering; thread-scaling run | GPU grid with `thrust::sort_by_key`, cell ranges, CUDA-event timings | Full benchmark + copy measurements on PACE; validation passing; first report figures |
| **Nov 11 - 17** | **M3: Tier 2 done** | Pheromone field, foraging, predators | Same rules as kernels, pheromone download | Mouse food/obstacles wired to both backends; overlay; tier-2 benchmark |
| **Nov 18 - 22** | **M4: Tier 3 + feature freeze (Sun Nov 22)** | State transfer on CPU | State transfer on GPU | Sliders/buttons/panel; switching tested both ways; FPS sweep |
| **Nov 23 - 29** | **M5: results, report, video** (Thanksgiving Nov 26) | Final CPU runs + section | Final GPU runs + section | Final runs, figures, readme, video, warning-free build, zip |

**Cut rule:** if Tier 1 (M2) is not working by **Thu Nov 12**, drop either foraging or predators from Tier 2 rather than squeezing Tier 3.

## How the work depends on each other

```
                SimTypes.h + Rules.h agreed (M0)  <-- blocks every real backend line
                 /                    \
   CPU flocking + grid (Ankitha)    DeviceBuffers + upload/download (Disha)
        |         \                        |
        |          reference metrics ----> CUDA flocking validated against CPU
        |                                  |
  ThreadPool scaling            GPU grid + cheap downloadRenderData
        \                                  |
         \------> benchmark + HUD numbers <--/     (Shravya: timing, copy, CSV, report)
                         |
     Tier 2 rules on CPU (Ankitha)  ---->  same rules as kernels (Disha)
                         |
     upload/downloadState on BOTH  ---->  live switching + full UI (Shravya)
                         |
              final PACE runs  ---->  report, video, zip
```

What each person is waiting on, and what to do meanwhile:

- **Shravya waits on** a real backend to render and measure. *Meanwhile:* the stub backend keeps the app, HUD, benchmark, and switching code testable, so nothing of yours is blocked.
- **Disha waits on** the exact rule formulas and the CPU reference metrics. *Meanwhile:* `DeviceBuffers`, upload/download, the grid build, and timing need no rules. Put the per-agent math in a shared header (`src/sim/Rules.h`, functions marked `SWARM_HD` = `__host__ __device__`) so both backends compute the same thing instead of two copies drifting apart.
- **Ankitha waits on** nothing after M0, which is why the CPU backend sits on the critical path. Tier 2 rules on the CPU should land a few days before Disha ports them.
- **Everyone waits on** `SimTypes.h`. Change it only in a small pull request that all three approve, and rebase right after it merges.

## Integration rules

- `main` must always build and run. Work on branches (`cpu/...`, `cuda/...`, `app/...`) and open a pull request; one teammate reviews.
- Merge small and often (at least twice a week each). A big merge in week 4 is the main schedule risk.
- Never edit another person's folder without telling them: `src/cpu/` = Ankitha, `src/cuda/` = Disha, `src/app/`, `src/bench/`, `scripts/` = Shravya, `src/sim/` = shared (Ankitha owns `SimTypes.h`).
- Results that go in the report come only from PACE-ICE runs via `scripts/pace/*.sbatch`, so every number has a node and a job ID.
