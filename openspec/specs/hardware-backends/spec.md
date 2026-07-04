# hardware-backends

## Purpose

Cyberfluids SHALL fully decouple fluid physics from the hardware driver so the same model
equations run on different compute targets without modification. It SHALL target a single
node (no MPI): a CPU backend using STL parallel algorithms, and optional GPU backends
(CUDA, Metal, OpenCL/SYCL). Switching the backend SHALL NOT change simulation results
beyond floating-point tolerance.

## Requirements

### Requirement: Backend abstraction seam
The library SHALL define a backend abstraction such that collision, streaming, and field
operations dispatch to the active backend, while model code (dynamics, boundary conditions)
is written once and is backend-agnostic.

#### Scenario: Switch backend without model changes
- **WHEN** a simulation is configured to use a different backend
- **THEN** the dynamics and boundary-condition code SHALL be unchanged and results SHALL
  match the reference backend within tolerance

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
The library SHALL provide optional, independently selectable GPU backends — NVIDIA CUDA,
Apple Metal (metal-cpp), and OpenCL/SYCL — each behind the backend abstraction and enabled
via build flags; absence of a GPU toolkit SHALL NOT break the CPU build.

#### Scenario: Build with a GPU backend disabled
- **WHEN** the library is built without any GPU toolkit present
- **THEN** the CPU backend SHALL build and run, and GPU backends SHALL be cleanly excluded

#### Scenario: GPU-accelerated run matches CPU
- **WHEN** the same simulation is run on a GPU backend and on the CPU backend
- **THEN** macroscopic fields SHALL match within a documented floating-point tolerance
