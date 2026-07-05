# Features

A capability-by-capability overview. Each links to its authoritative OpenSpec spec. Status
reflects **implementation**, not specification (all specs exist).

Legend: ✅ implemented · 🟡 partial (MVP subset) · 📋 planned.

| Capability | Summary | Status |
|---|---|---|
| [NumPP/SciPP foundation](../openspec/specs/numpp-scipp-foundation/spec.md) | All arrays/tensors via NumPP; scientific algorithms via SciPP; no generic third-party math libs | 🟡 NumPP integrated; SciPP wired, unused |
| [Lattice descriptors](../openspec/specs/lattice-descriptors/spec.md) | Compile-time `DdQq` stencils, Concept-validated | 🟡 D2Q9/D3Q19/D3Q27/D2Q5/D3Q7 + forced/advected external-field variants (WithSource planned) |
| [Core data structures](../openspec/specs/core-data-structures/spec.md) | `BlockLattice`, `Cell` views, scalar/tensor fields; smart-pointer ownership | ✅ Implemented |
| [External fields](../openspec/specs/external-fields/spec.md) | Per-cell external scalars (force / advection velocity), SoA, fluid→AD coupling | ✅ Implemented (zero-cost when absent) |
| [Collision dynamics](../openspec/specs/collision-dynamics/spec.md) | BGK, TRT, MRT, regularized, forced, advection-diffusion; `omega = 1/tau` | ✅ BGK · TRT · MRT (D2Q9 **and D3Q19**) · regularized · forced (uniform + per-cell) · advection-diffusion |
| [Streaming & time step](../openspec/specs/streaming-and-timestep/spec.md) | collide / stream / fused collideAndStream | ✅ Implemented |
| [Boundary conditions](../openspec/specs/boundary-conditions/spec.md) | Bounce-back, Zou/He velocity, periodic, off-lattice STL | ✅ bounce-back · moving-wall · Zou/He · periodic · off-lattice STL (partial bounce-back) |
| [Hardware backends](../openspec/specs/hardware-backends/spec.md) | CPU `par_unseq`; optional CUDA / Metal / OpenCL / SYCL; no MPI | ✅ CPU · CUDA · OpenCL (D3Q19 wind tunnel, validated ~1e-5 vs fp64 oracle) · Metal (D2Q9 cavity); SYCL planned |
| [Physical models](../openspec/specs/physical-models/spec.md) | Navier-Stokes, Shan-Chen multiphase, porous media, thermal | ✅ Navier-Stokes · Shan-Chen multiphase (single + multi-component) · thermal Boussinesq · porous media (partial bounce-back) |
| [Geometry & I/O](../openspec/specs/geometry-and-io/spec.md) | STL import + voxelization, VTK output, checkpoint/restart | ✅ VTK + checkpoint/restart + centerline CSV + STL/OBJ import & voxelization (CyberMeshGenerator) |
| [Language bindings](../openspec/specs/language-bindings/spec.md) | Python (NumPy interop) and Swift | ✅ Both, over a shared C ABI; zero-copy DLPack; 3D wind-tunnel binding |
| [Platform support](../openspec/specs/platform-support/spec.md) | Desktop/server, iOS/iPadOS, Android NDK | 🟡 desktop/server (mobile toolchains planned) |
| [Oracle validation](../openspec/specs/oracle-validation/spec.md) | Regression vs Palabos within a documented tolerance | ✅ 2D + 3D lid-driven cavity vs Palabos (~0.8% RMS of U) |
| [Consumability](../openspec/specs/consumability/spec.md) | Usable-by-others readiness rubric (install, packaging, CI, versioning, governance) | ✅ find_package + pip wheel + zero-setup build + CI + governance files |

## What is implemented

Each capability is delivered as its own OpenSpec change (see
[`openspec/changes/archive/`](../openspec/changes/archive)); **36/36 tests pass**.

- **Dynamics & stencils:** D2Q9 / D3Q19 / D3Q27 / D2Q5 / D3Q7; BGK, TRT, MRT (2D + 3D),
  regularized, and forced (Guo) collision; per-cell external force / advection-velocity fields.
- **Physics:** lid-driven cavity, forced Poiseuille, Shan-Chen multiphase (single + multi-component),
  thermal Boussinesq with two-way advection-diffusion coupling, Rayleigh-Bénard, and porous media
  via partial bounce-back (analytic Darcy oracle).
- **Geometry & I/O:** STL/OBJ import + voxelization (CyberMeshGenerator), off-lattice no-slip via
  partial bounce-back, VTK output, and bit-exact checkpoint/restart.
- **Solvers & bindings:** a 3D wind tunnel (external flow past an imported mesh), driven from Python
  with zero-copy DLPack interop and VTK export; Python + Swift over a shared C ABI.
- **Backends & validation:** CPU (`par_unseq` + serial fallback), CUDA and OpenCL (D3Q19
  wind-tunnel device kernels, ~30× the parallel CPU on an RTX 5060 — see
  [benchmarks](benchmarks.md)), and Metal (Apple Silicon); a 2D + 3D Palabos oracle; a GitHub
  Actions CI pipeline.

- **Planned follow-ups:** SYCL device kernels (seam locked), iOS / Android toolchains, and
  momentum-exchange drag/lift reporting. Each becomes its own OpenSpec change.
