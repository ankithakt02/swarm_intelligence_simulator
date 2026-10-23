/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Creates backends by name ("cpu", "cuda", "stub"). Backends that were not
compiled in (for example "cuda" on a machine without nvcc) are simply not
listed by availableBackends().
*/

#pragma once

#include "sim/SimTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace swarm
{

/// Names of the backends compiled into this build, best first ("cuda", "cpu", "stub").
std::vector<std::string> availableBackends();

/// Returns nullptr if the name is unknown or that backend was not compiled in.
std::unique_ptr<ISimBackend> makeBackend(const std::string& name);

/// The backend the GUI starts with when none is requested: "cpu" if present, else "stub".
std::string defaultBackend();

}  // namespace swarm
