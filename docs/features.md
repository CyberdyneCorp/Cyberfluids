# Features

A capability-by-capability overview. Each links to its authoritative OpenSpec spec. Status
reflects **implementation**, not specification (all specs exist).

Legend: ✅ implemented · 🟡 partial (MVP subset) · 📋 planned.

| Capability | Summary | Status |
|---|---|---|
| [NumPP/SciPP foundation](../openspec/specs/numpp-scipp-foundation/spec.md) | All arrays/tensors via NumPP; scientific algorithms via SciPP; no generic third-party math libs | 🟡 NumPP integrated; SciPP wired, unused |
| [Lattice descriptors](../openspec/specs/lattice-descriptors/spec.md) | Compile-time `DdQq` stencils, Concept-validated | 🟡 D2Q9, D3Q19, D3Q27, D2Q5, D3Q7 (external-field variants planned) |
| [Core data structures](../openspec/specs/core-data-structures/spec.md) | `BlockLattice`, `Cell` views, scalar/tensor fields; smart-pointer ownership | ✅ Implemented |
| [Collision dynamics](../openspec/specs/collision-dynamics/spec.md) | BGK, TRT, MRT, regularized, forced; `omega = 1/tau` | 🟡 BGK · TRT · MRT (D2Q9) · regularized · forced (uniform) — MRT-3D & per-cell forcing planned |
| [Streaming & time step](../openspec/specs/streaming-and-timestep/spec.md) | collide / stream / fused collideAndStream | ✅ Implemented |
| [Boundary conditions](../openspec/specs/boundary-conditions/spec.md) | Bounce-back, Zou/He velocity, periodic, off-lattice STL | 🟡 bounce-back, moving-wall, Zou/He (top), periodic (STL/all-faces planned) |
| [Hardware backends](../openspec/specs/hardware-backends/spec.md) | CPU `par_unseq`; optional CUDA / Metal / OpenCL / SYCL; no MPI | 🟡 CPU implemented; GPU stubs |
| [Physical models](../openspec/specs/physical-models/spec.md) | Navier-Stokes, Shan-Chen multiphase, porous media, thermal | 🟡 Navier-Stokes cavity (others planned) |
| [Geometry & I/O](../openspec/specs/geometry-and-io/spec.md) | STL import + voxelization, VTK output, checkpoint/restart | 🟡 centerline CSV (STL/VTK/checkpoint planned) |
| [Language bindings](../openspec/specs/language-bindings/spec.md) | Python (NumPy interop) and Swift | ✅ Both, over a shared C ABI |
| [Platform support](../openspec/specs/platform-support/spec.md) | Desktop/server, iOS/iPadOS, Android NDK | 🟡 desktop/server (mobile toolchains planned) |

## What is implemented (MVP)

The [`bootstrap-cyberfluids-core`](../openspec/changes/bootstrap-cyberfluids-core) change
delivers a working, provably-correct slice:

- **Done:** D2Q9 + D3Q19, BGK, collide-and-stream, bounce-back + moving-wall + Zou/He +
  periodic boundaries, the CPU backend, a 2D+3D lid-driven cavity **validated against Palabos**
  (~0.8% RMS of the lid speed), Python + Swift bindings, and the CMake build. 12/12 tests pass.
- **Planned follow-ups:** TRT/MRT/regularized/forced, D3Q27 & AD descriptors, GPU backends,
  multiphase, thermal, porous media, STL import + VTK + checkpointing, mobile toolchains, and a
  3D oracle. Each becomes its own OpenSpec change.
