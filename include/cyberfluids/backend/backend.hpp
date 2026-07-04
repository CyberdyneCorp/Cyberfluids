#pragma once

#include "cyberfluids/backend/cpu.hpp"
#include "cyberfluids/backend/gpu_stubs.hpp"

namespace cyberfluids::backend {

/// The default compute backend for this build. CPU is always available; GPU
/// backends plug in here behind their feature flags. Model code (dynamics,
/// streaming) is written against the `Backend::forEachIndex` seam and never
/// changes when the backend does. See docs/backends.md.
using Default = Cpu;

}  // namespace cyberfluids::backend
