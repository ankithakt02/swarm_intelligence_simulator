# Real-Time Swarm Intelligence Simulator (Multithreaded CPU + CUDA)

ECE 4122/6122 final project. An interactive C++17 / SFML sandbox where 100,000+ agents show emergent
behaviour (boid flocking, ant foraging with a pheromone field, predators hunting prey), simulated by two
interchangeable backends: a `std::thread` CPU backend and a CUDA backend, switchable at runtime.

**Team:** Ankitha Kottamthody (CPU backend), Disha Jagadeesha (CUDA backend), Shravya Vasudeva (application, UI, benchmarking, build)

> Status: Tier 0 in place. The app, HUD, benchmark, validation, and PACE-ICE scripts build and run against a
> placeholder `stub` backend until the CPU and CUDA backends land. See [docs/PLAN.md](docs/PLAN.md).

## Tested platforms and libraries

| | Version | Link |
|---|---|---|
| OS | Ubuntu 24.04 (dev), RHEL 9 (PACE-ICE) | |
| Compiler | GCC 13 (C++17) | |
| CMake | >= 3.18 | https://cmake.org/ |
| SFML | 2.6.x (graphics, window, system) | https://www.sfml-dev.org/ / https://github.com/SFML/SFML/tree/2.6.2 |
| CUDA Toolkit | *fill in exact version from PACE-ICE `nvcc --version`* | https://developer.nvidia.com/cuda-toolkit |
| Thrust | ships with the CUDA Toolkit | https://nvidia.github.io/cccl/thrust/ |
| Font | DejaVu Sans Mono (bundled in `assets/fonts`, Bitstream Vera license) | https://dejavu-fonts.github.io/ |

## Repository layout

```
src/sim/      shared model: SimTypes.h (ISimBackend, SoA agents, params, world), Rules.h, metrics, factory
src/cpu/      CPU backend (Ankitha)          -> registered as "cpu" once files exist
src/cuda/     CUDA backend (Disha)           -> registered as "cuda" when nvcc is found
src/app/      SFML app: Application, Renderer, InputHandler, Hud, SimulationController, FPS sweep (Shravya)
src/bench/    headless BenchmarkRunner, MetricsLogger, timing (Shravya)
scripts/pace/ PACE-ICE build and Slurm job scripts
scripts/plot_results.py   CSVs -> report figures + Markdown tables
docs/         PLAN.md (tiers, timeline, dependencies), REPORT_TARGETS.md (measurable results)
results/      CSV output; results/final/ holds the runs cited in the report
```

## Build

```bash
# Full build (interactive app + benchmark). Uses an installed SFML 2.6, otherwise downloads it.
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Benchmark only (no SFML), e.g. on PACE-ICE batch nodes
cmake -S . -B build -DSWARM_BUILD_GUI=OFF
```

CUDA is detected automatically; force CPU-only with `-DSWARM_ENABLE_CUDA=OFF`. Set the GPU architecture with
`-DCMAKE_CUDA_ARCHITECTURES=70` (V100), `75` (RTX 6000), `80` (A100), `90` (H100).

If SFML has to be downloaded on Linux, its dependencies are needed: X11, Xrandr, Xcursor, Xi, udev, OpenGL,
FreeType (Ubuntu: `sudo apt install libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev libfreetype-dev`).

## Run

```bash
./build/swarm                                   # cpu backend if built, else stub; 20k boids
./build/swarm --backend cuda --agents 100k
./build/swarm --agents 50k --ants-frac 0.2 --pred-frac 0.002
./build/swarm --help
```

| Input | Action |
|---|---|
| Left click | drop food |
| Left click + drag | draw an obstacle wall |
| Right click | erase nearby food / obstacles |
| Space / R | pause / reset |
| B | switch backend (state is transferred, the swarm continues) |
| Up / Down | CPU thread count |
| + / - | double / halve agent count |
| P / O / H | triangles vs points / pheromone overlay / HUD |
| C / Esc | clear world / quit |

The HUD shows FPS and the per-frame breakdown: **step**, **copy** (backend -> host render data, measured
separately as the TA asked), **fill** (vertex array), and **draw** (draw + display).

## Benchmarks and validation

```bash
./build/swarm_bench --backends cpu --agents 50k --threads 1,2,4,8      # thread scaling
./build/swarm_bench --backends cpu,cuda --agents 10k,50k,100k --threads 24   # CPU vs GPU (+ copy timing)
./build/swarm_bench --validate --reference cpu --candidates cuda       # CPU/GPU agreement, 5% tolerance
./build/swarm --fps-sweep 10k,25k,50k,100k,200k --sweep-backends cpu,cuda    # largest count at 30/60 FPS
python3 scripts/plot_results.py --bench results/bench*.csv --fps results/fps_sweep.csv \
        --validation results/validation*.csv --out results/figures
```

What we measure and the targets: [docs/REPORT_TARGETS.md](docs/REPORT_TARGETS.md).

## Running on PACE-ICE

```bash
git clone <repo-url> swarm-sim && cd swarm-sim
salloc --gres=gpu:1 -t 0:30:00          # build on a GPU node so the CUDA backend is compiled in
bash scripts/pace/build.sh
exit
sbatch scripts/pace/bench_cpu.sbatch     # thread scaling      -> results/bench_cpu_<job>.csv
sbatch scripts/pace/bench_gpu.sbatch     # CPU vs GPU + copy + validation -> results/bench_gpu_<job>.csv
squeue -u $USER                          # logs land in results/logs/
```

Module names change between semesters; check `module avail gcc cuda cmake` and edit `scripts/pace/env.sh`.
The interactive window and the FPS sweep need a display, so run those on a machine with a monitor and an
NVIDIA GPU (or a PACE-ICE OnDemand desktop with a GPU), not in a batch job.

## Team workflow

- `main` always builds. Branch per task (`cpu/grid`, `cuda/sort`, `app/sliders`), open a pull request, one review.
- Folder ownership: `src/cpu` Ankitha, `src/cuda` Disha, `src/app` + `src/bench` + `scripts` Shravya,
  `src/sim` shared (change `SimTypes.h` only in a small PR all three approve).
- Every source file starts with the course header (author, class, date, description) and must compile without warnings.
- Keep the repository **private** (course academic-integrity policy).

## Submission

`Final_Project.zip` contains the source, CMakeLists.txt, this readme, and the report; the demo video link goes
in the report.
