# CPU backend (owner: Ankitha)

Every `.cpp` in this folder is compiled automatically (CMake globs it) and defines
`SWARM_HAVE_CPU_BACKEND`, which registers the backend as `cpu`.

Contract (see `src/sim/SimTypes.h`):

- `src/cpu/CpuBackend.h` declares `class swarm::CpuBackend : public ISimBackend`, default-constructible.
- `name()` returns `"cpu"`; `supportsThreads()` returns `true`; `setThreadCount(0)` means all cores.
- `step()` must be data-race free (double-buffered state, no locks in the update loop).
- Fill `lastTimings()` (grid build / update / pheromone, ms) so the benchmark can report the breakdown.
- Rule math goes in `src/sim/Rules.h` so the CUDA kernels can call the same functions.

Planned files: `CpuBackend.h/.cpp`, `SpatialGrid.h/.cpp`, `ThreadPool.h/.cpp`, `PheromoneField.h/.cpp`.

Quick check after building: `./build/swarm_bench --list-backends` should print `cpu`.
