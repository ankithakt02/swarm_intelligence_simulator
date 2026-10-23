/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Backend registry. CMake defines SWARM_HAVE_CPU_BACKEND when src/cpu/ has
sources and SWARM_HAVE_CUDA_BACKEND when src/cuda/ has sources and CUDA is
enabled. Each backend must provide a default-constructible class:
    src/cpu/CpuBackend.h    -> swarm::CpuBackend
    src/cuda/CudaBackend.h  -> swarm::CudaBackend  (plain C++ header, no CUDA syntax)
*/

#include "sim/BackendFactory.h"

#include "sim/StubBackend.h"

#ifdef SWARM_HAVE_CPU_BACKEND
#include "cpu/CpuBackend.h"
#endif

#ifdef SWARM_HAVE_CUDA_BACKEND
#include "cuda/CudaBackend.h"
#endif

namespace swarm
{

std::vector<std::string> availableBackends()
{
    std::vector<std::string> names;
#ifdef SWARM_HAVE_CUDA_BACKEND
    names.emplace_back("cuda");
#endif
#ifdef SWARM_HAVE_CPU_BACKEND
    names.emplace_back("cpu");
#endif
    names.emplace_back("stub");
    return names;
}

std::unique_ptr<ISimBackend> makeBackend(const std::string& name)
{
#ifdef SWARM_HAVE_CPU_BACKEND
    if (name == "cpu")
    {
        return std::make_unique<CpuBackend>();
    }
#endif
#ifdef SWARM_HAVE_CUDA_BACKEND
    if (name == "cuda")
    {
        return std::make_unique<CudaBackend>();
    }
#endif
    if (name == "stub")
    {
        return std::make_unique<StubBackend>();
    }
    return nullptr;
}

std::string defaultBackend()
{
#ifdef SWARM_HAVE_CPU_BACKEND
    return "cpu";
#else
    return "stub";
#endif
}

}  // namespace swarm
