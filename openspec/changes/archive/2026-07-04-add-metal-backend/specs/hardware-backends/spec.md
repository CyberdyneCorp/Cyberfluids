## MODIFIED Requirements

### Requirement: Optional GPU backends
The library SHALL provide optional, independently selectable GPU backends — NVIDIA CUDA, Apple
Metal, and OpenCL/SYCL — enabled via build flags; absence of a GPU toolkit SHALL NOT break the CPU
build. Because Metal Shading Language has no double type, GPU paths run in single precision, so
GPU-vs-CPU agreement is bounded by an fp32 tolerance rather than being bit-exact. The Apple Metal
backend SHALL be implemented against the system Metal framework (no external dependency) and, for
the MVP, provides a D2Q9 BGK lid-driven cavity GPU solver; other descriptors/models remain CPU-only.

#### Scenario: Build with a GPU backend disabled
- **WHEN** the library is built without any GPU backend flag enabled
- **THEN** the CPU backend SHALL build and run, and GPU backends SHALL be cleanly excluded

#### Scenario: GPU-accelerated run matches CPU
- **WHEN** the same D2Q9 lid-driven cavity is run on the Metal backend and on the CPU backend with
  identical parameters and boundary scheme
- **THEN** the steady-state velocity field SHALL agree within a documented fp32 tolerance
  (a fraction of the lid speed)

#### Scenario: Deterministic GPU run
- **WHEN** the same Metal simulation is run twice
- **THEN** the two runs SHALL produce bit-identical results (per-cell kernels use no atomics or
  cross-thread reductions)
