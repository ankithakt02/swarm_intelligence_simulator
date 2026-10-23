# CUDA backend (owner: Disha)

Every `.cu` / `.cpp` in this folder is compiled automatically when CMake finds nvcc,
and defines `SWARM_HAVE_CUDA_BACKEND`, which registers the backend as `cuda`.

Contract (see `src/sim/SimTypes.h`):

- `src/cuda/CudaBackend.h` declares `class swarm::CudaBackend : public ISimBackend`,
  default-constructible. **This header must be plain C++** (no `__global__`, no CUDA
  types in it) because the app and benchmark include it from `.cpp` files; keep device
  pointers inside `DeviceBuffers` / a pimpl in the `.cu` file.
- `name()` returns `"cuda"`; `deviceName()` returns the GPU name from `cudaGetDeviceProperties`.
- `step()` may be asynchronous; `synchronize()` must wait for it (`cudaDeviceSynchronize` or a stream sync),
  because the benchmark and HUD time on the host around `step()` + `synchronize()`.
- `downloadRenderData()` is called **every rendered frame**: copy only x, y, vx, vy, type,
  ideally into pinned host memory (`cudaMallocHost`). The TA asked for this copy to be measured
  separately; the benchmark already times it (`copy_ms_*` columns).
- Fill `lastTimings()` from CUDA events (grid build / update / pheromone).
- Call the shared rule functions in `src/sim/Rules.h` (marked `SWARM_HD`) from the kernels.

Planned files: `CudaBackend.h`, `CudaBackend.cu`, `DeviceBuffers.h`, `Kernels.cu`, `CudaCheck.h`.

Build on PACE-ICE on a GPU node: `bash scripts/pace/build.sh`; then `./build-pace/swarm_bench --list-backends` should print `cuda`.
