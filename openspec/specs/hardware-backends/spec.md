# hardware-backends

## Purpose

Cyberfluids SHALL fully decouple fluid physics from the hardware driver so the same model
equations run on different compute targets without modification. It SHALL target a single
node (no MPI): a CPU backend using STL parallel algorithms, and optional GPU backends
(CUDA, Metal, OpenCL/SYCL). Switching the backend SHALL NOT change simulation results
beyond floating-point tolerance.
## Requirements
### Requirement: Backend abstraction seam
The library SHALL expose a compute seam so per-cell CPU work is written once and runs across
the index range via `Backend::forEachIndex(n, f)`. This seam is **CPU-only**: `f` is a host
callable, so it lowers to an STL parallel-for and CANNOT dispatch to a GPU. GPU acceleration
SHALL therefore be provided as **bespoke device-kernel solvers** (the hot collide + stream loop
rewritten in device code, data kept resident on the device), NOT through this seam. The GPU
stub structs that satisfy the seam by running the host callable serially SHALL NOT be presented
as the acceleration path.

#### Scenario: CPU kernel runs across the seam
- **WHEN** a per-cell operation is expressed as `Backend::forEachIndex(n, f)` on the CPU backend
- **THEN** it SHALL run across `[0, n)` using STL parallel algorithms (or a serial fallback)

#### Scenario: GPU acceleration bypasses the seam
- **WHEN** a solver is GPU-accelerated
- **THEN** it SHALL use dedicated device kernels validated against the CPU solver, not
  `forEachIndex` dispatch of a host callable

### Requirement: CPU backend via STL parallel algorithms
The library SHALL provide a CPU backend that parallelizes cell operations across all cores
using `std::execution::par_unseq`, with no external parallelism dependency, and code shaped
to enable compiler auto-vectorization (AVX-512, ARM Neon).

#### Scenario: Multi-core CPU execution
- **WHEN** a simulation runs on the CPU backend on a multi-core machine
- **THEN** collide-and-stream work SHALL be distributed across available cores via
  `std::execution::par_unseq`

### Requirement: Single-node, no MPI
The library SHALL NOT depend on MPI; all parallelism SHALL be intra-node (threads/SIMD/GPU).

#### Scenario: No MPI dependency
- **WHEN** the build configuration and dependency graph are inspected
- **THEN** no MPI library or MPI launcher SHALL be required to build or run a simulation

### Requirement: Optional GPU backends
The library SHALL provide optional, independently selectable GPU backends — NVIDIA CUDA, Apple
Metal, and OpenCL/SYCL — enabled via build flags; absence of a GPU toolkit SHALL NOT break the CPU
build. Because GPU paths run in single precision, GPU-vs-CPU agreement is bounded by an fp32
tolerance rather than being bit-exact. The Apple Metal backend SHALL provide a D2Q9 BGK
lid-driven cavity GPU solver against the system Metal framework. The library SHALL ALSO provide a
**3D wind-tunnel (D3Q19) GPU solver** with **OpenCL** device kernels (validated against the fp64
CPU `WindTunnel3D` oracle) and a **CUDA** mirror of the same kernels; each GPU solver SHALL keep
populations resident on the device across steps and copy back only for readout.

#### Scenario: Build with a GPU backend disabled
- **WHEN** the library is built without any GPU backend flag enabled
- **THEN** the CPU backend SHALL build and run, and GPU backends SHALL be cleanly excluded

#### Scenario: GPU-accelerated run matches CPU
- **WHEN** the same D2Q9 lid-driven cavity (Metal) or 3D wind tunnel (OpenCL/CUDA) is run on the
  GPU and on the CPU with identical parameters
- **THEN** the steady-state velocity field SHALL agree within a documented fp32 tolerance

#### Scenario: Deterministic GPU run
- **WHEN** the same GPU simulation is run twice on the same device
- **THEN** the two runs SHALL produce bit-identical population fields

