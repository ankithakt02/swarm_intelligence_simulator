# ECE 4122/6122 Final Project - Real-Time Swarm Intelligence Simulator
# Team: Ankitha Kottamthody, Disha Jagadeesha, Shravya Vasudeva
#
# Targets
#   swarm_core   static library: simulation model, backends, metrics, benchmark
#   swarm_bench  headless benchmark / validation executable (no SFML; runs on PACE-ICE nodes)
#   swarm        interactive SFML application
#
# Options
#   -DSWARM_BUILD_GUI=OFF     build only the headless benchmark (e.g. on PACE-ICE batch nodes)
#   -DSWARM_ENABLE_CUDA=OFF   force a CPU-only build even if nvcc is available

cmake_minimum_required(VERSION 3.18)
project(SwarmSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SWARM_BUILD_GUI "Build the SFML interactive application" ON)
option(SWARM_ENABLE_CUDA "Build the CUDA backend if a CUDA compiler is found" ON)

# ---------------------------------------------------------------------------
# CUDA (optional): the project still builds and runs CPU-only without it.
# ---------------------------------------------------------------------------
set(SWARM_HAVE_CUDA OFF)
if(SWARM_ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        set(CMAKE_CUDA_STANDARD 17)
        set(CMAKE_CUDA_STANDARD_REQUIRED ON)
        if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
            # V100 = 70, RTX 6000 = 75, A100 = 80, H100 = 90. Override with -DCMAKE_CUDA_ARCHITECTURES=...
            set(CMAKE_CUDA_ARCHITECTURES 70 75 80)
        endif()
        find_package(CUDAToolkit REQUIRED)
        set(SWARM_HAVE_CUDA ON)
        message(STATUS "CUDA backend: ENABLED (${CMAKE_CUDA_COMPILER_VERSION})")
    else()
        message(STATUS "CUDA backend: DISABLED (no CUDA compiler found)")
    endif()
else()
    message(STATUS "CUDA backend: DISABLED (SWARM_ENABLE_CUDA=OFF)")
endif()

# ---------------------------------------------------------------------------
# Warnings (the final submission must compile without warnings)
# ---------------------------------------------------------------------------
if(MSVC)
    set(SWARM_WARNINGS /W4 /permissive-)
else()
    set(SWARM_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion)
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Core library (no SFML dependency)
# ---------------------------------------------------------------------------
add_library(swarm_core STATIC
    src/sim/SimTypes.cpp
    src/sim/Metrics.cpp
    src/sim/StubBackend.cpp
    src/sim/BackendFactory.cpp
    src/bench/BenchmarkRunner.cpp
    src/bench/MetricsLogger.cpp
    src/app/SimulationController.cpp
)

# CPU backend (Ankitha): every .cpp in src/cpu/ is picked up automatically.
file(GLOB SWARM_CPU_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/cpu/*.cpp)
target_sources(swarm_core PRIVATE ${SWARM_CPU_SOURCES})
if(SWARM_CPU_SOURCES)
    target_compile_definitions(swarm_core PUBLIC SWARM_HAVE_CPU_BACKEND=1)
endif()

# CUDA backend (Disha): every .cu/.cpp in src/cuda/ is picked up when CUDA is enabled.
if(SWARM_HAVE_CUDA)
    file(GLOB SWARM_CUDA_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cuda/*.cu
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cuda/*.cpp)
    target_sources(swarm_core PRIVATE ${SWARM_CUDA_SOURCES})
    if(SWARM_CUDA_SOURCES)
        target_compile_definitions(swarm_core PUBLIC SWARM_HAVE_CUDA_BACKEND=1)
    endif()
    target_link_libraries(swarm_core PUBLIC CUDA::cudart)
    set_target_properties(swarm_core PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif()

target_include_directories(swarm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(swarm_core PUBLIC Threads::Threads)
target_compile_options(swarm_core PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${SWARM_WARNINGS}>)

# ---------------------------------------------------------------------------
# Headless benchmark executable
# ---------------------------------------------------------------------------
add_executable(swarm_bench src/main_bench.cpp)
target_link_libraries(swarm_bench PRIVATE swarm_core)
target_compile_options(swarm_bench PRIVATE ${SWARM_WARNINGS})

# ---------------------------------------------------------------------------
# Interactive SFML application
# ---------------------------------------------------------------------------
if(SWARM_BUILD_GUI)
    # Prefer an installed SFML 2.6; otherwise download and build it (needed on PACE-ICE).
    find_package(SFML 2.6 COMPONENTS graphics window system QUIET)
    if(SFML_FOUND)
        message(STATUS "SFML: using installed ${SFML_VERSION}")
    else()
        message(STATUS "SFML: not installed, fetching SFML 2.6.2 from GitHub")
        include(FetchContent)
        set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
        set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(SFML
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG 2.6.2
            GIT_SHALLOW ON
            SYSTEM)
        FetchContent_MakeAvailable(SFML)
    endif()

    add_executable(swarm
        src/main_gui.cpp
        src/app/Application.cpp
        src/app/Renderer.cpp
        src/app/InputHandler.cpp
        src/app/Hud.cpp
        src/app/FpsSweep.cpp
    )
    target_link_libraries(swarm PRIVATE swarm_core sfml-graphics sfml-window sfml-system)
    # The HUD font ships in assets/fonts so the app works on machines without system fonts (PACE-ICE).
    target_compile_definitions(swarm PRIVATE SWARM_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    target_compile_options(swarm PRIVATE ${SWARM_WARNINGS})
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}  GUI: ${SWARM_BUILD_GUI}  CUDA: ${SWARM_HAVE_CUDA}")
