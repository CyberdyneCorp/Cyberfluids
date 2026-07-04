## Why

Metal is the acceleration pillar of the project's "runs on GPUs and mobile" mission, but the
Metal backend is only a stub. The CPU seam (`Backend::forEachIndex` running a host C++ lambda with
virtual dispatch) cannot run on a GPU, so a real Metal path needs device buffers + compute kernels.
This adds an MVP: a D2Q9 BGK lid-driven cavity running on the Apple Silicon GPU that matches the
CPU solver within an fp32 tolerance — proving the GPU path end-to-end.

## What Changes

- Add a **GPU solver variant** `solver::MetalLidDrivenCavity2D` mirroring the public API of
  `LidDrivenCavity2D<>` (ctor, run, nx/ny, density, velocity), holding the `{q, ncells}`
  populations in Metal buffers (fp32, unified memory) and running **MSL compute kernels** for BGK
  collide + the cavity pull-stream (bounce-back walls + moving-wall lid), compiled at runtime.
- Implement it in **Objective-C++** (`.mm`) against the system `<Metal/Metal.h>` — no external
  metal-cpp download; guarded by `CYBERFLUIDS_METAL` (Apple only) via a pimpl so Metal headers stay
  out of the public API. The CPU path is untouched; the two solvers coexist as independent types.
- Validate: run-to-run determinism; and CPU(fp64) vs Metal(fp32) steady-state cavity fields agree
  within a documented tolerance (a fraction of the lid speed).

## Capabilities

### Modified Capabilities
- `hardware-backends`: the **Optional GPU backends** requirement is updated — Metal is implemented
  (via Objective-C++ + the system Metal framework) as a GPU solver for the D2Q9 BGK cavity in fp32,
  matching CPU within an fp32 tolerance; CUDA/OpenCL and other models remain planned.

## Impact

- New: `include/cyberfluids/solver/metal/lid_driven_cavity_metal.hpp` (pimpl, guarded),
  `src/metal/lid_driven_cavity_metal.mm` (device/kernels/dispatch), `cmake/CyberfluidsMetal.cmake`,
  a Metal test. CMake links `-framework Metal -framework Foundation` only when `CYBERFLUIDS_METAL`
  is ON and `APPLE`. No change to CPU code, the C ABI, or existing solvers.

## Non-Goals

- Other descriptors/models on GPU (only D2Q9 BGK cavity); CUDA/OpenCL/SYCL; fp64 on GPU (MSL has
  no double — the GPU path is fp32 by construction); a `Backend`-templated GPU seam.
