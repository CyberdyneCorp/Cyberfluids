# Features

A capability-by-capability overview. Each links to its authoritative OpenSpec spec. Status
reflects **implementation**, not specification (all specs exist).

| Capability | Summary | Status |
|---|---|---|
| [NumPP/SciPP foundation](../openspec/specs/numpp-scipp-foundation/spec.md) | All arrays/tensors via NumPP; scientific algorithms via SciPP; no generic third-party math libs | 📋 Planned |
| [Lattice descriptors](../openspec/specs/lattice-descriptors/spec.md) | Compile-time `DdQq` stencils, Concept-validated: D2Q9, D3Q19, D3Q27, D2Q5, D3Q7 | 📋 Planned |
| [Core data structures](../openspec/specs/core-data-structures/spec.md) | `BlockLattice`, `Cell` views, scalar/tensor fields; smart-pointer ownership | 📋 Planned |
| [Collision dynamics](../openspec/specs/collision-dynamics/spec.md) | BGK, TRT, MRT, regularized, forced; `omega = 1/tau` | 📋 Planned |
| [Streaming & time step](../openspec/specs/streaming-and-timestep/spec.md) | collide-and-stream over C++20 Ranges | 📋 Planned |
| [Boundary conditions](../openspec/specs/boundary-conditions/spec.md) | Bounce-back, Zou/He velocity & pressure, periodic, off-lattice STL | 📋 Planned |
| [Hardware backends](../openspec/specs/hardware-backends/spec.md) | CPU `par_unseq`; optional CUDA / Metal / OpenCL / SYCL; no MPI | 📋 Planned |
| [Physical models](../openspec/specs/physical-models/spec.md) | Navier-Stokes, Shan-Chen multiphase, porous media, thermal (Boussinesq) | 📋 Planned |
| [Geometry & I/O](../openspec/specs/geometry-and-io/spec.md) | STL import + voxelization, VTK output, checkpoint/restart | 📋 Planned |
| [Language bindings](../openspec/specs/language-bindings/spec.md) | Python (NumPy interop) and Swift | 📋 Planned |
| [Platform support](../openspec/specs/platform-support/spec.md) | Desktop/server, iOS/iPadOS, Android NDK | 📋 Planned |

## What lands first (MVP)

The [`bootstrap-cyberfluids-core`](../openspec/changes/bootstrap-cyberfluids-core) change
delivers the smallest provably-correct slice:

- **In:** D2Q9 + D3Q19, BGK, collide-and-stream, bounce-back + Zou/He, CPU backend, a 2D+3D
  lid-driven cavity validated against Palabos, Python + Swift scaffolds, the CMake build.
- **Deferred:** TRT/MRT/regularized/forced, D3Q27 & AD descriptors, GPU backends, multiphase,
  thermal, porous media, STL import, checkpointing. Each becomes a follow-up change.

Legend: 📋 Planned · 🚧 In progress · ✅ Implemented
