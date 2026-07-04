# Roadmap

Milestones are delivered as OpenSpec changes. Each ships with tests (including Palabos-oracle
regressions where applicable). This is a plan, not a commitment of dates.

```mermaid
flowchart LR
    M0["M0 · Bootstrap MVP ✅"] --> M1["M1 · Dynamics & stencils"]
    M1 --> M2["M2 · Geometry & I/O"]
    M2 --> M3["M3 · GPU backends"]
    M3 --> M4["M4 · Multiphysics"]
    M4 --> M5["M5 · Mobile & bindings GA"]
```

## M0 — Bootstrap MVP  *(done — 12/12 tests, Palabos-validated)*

[`bootstrap-cyberfluids-core`](../openspec/changes/bootstrap-cyberfluids-core): CMake +
NumPP, D2Q9+D3Q19, BGK, collide-and-stream, bounce-back + moving-wall + Zou/He + periodic,
CPU backend, 2D+3D lid-driven cavity, Palabos oracle regression (~0.8% RMS of U), Python +
Swift bindings. Deferred within M0: generic multi-face velocity BC, iOS/Android toolchains,
and a 3D oracle reference.

## M1 — Dynamics & stencils

TRT, MRT, regularized, and forced dynamics; D3Q27 and D2Q5/D3Q7 advection-diffusion
descriptors.

## M2 — Geometry & I/O

STL import + voxelization, off-lattice bounce-back on imported geometries, VTK output,
checkpoint/restart.

## M3 — GPU backends

Metal (Apple Silicon) and CUDA (NVIDIA), then OpenCL/SYCL — each behind the backend seam,
validated against the CPU path.

## M4 — Multiphysics

Shan-Chen multi-component/multiphase, porous media (partial bounce-back), thermal flow
(Boussinesq + advection-diffusion coupling).

## M5 — Mobile & bindings GA

iOS/iPadOS and Android NDK builds hardened; Python and Swift bindings to general
availability with full field interop.

See open capabilities in [features](features.md) and the target contract in
[`../openspec/specs/`](../openspec/specs).
