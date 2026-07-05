# Roadmap

Milestones are delivered as OpenSpec changes. Each ships with tests (including Palabos-oracle
regressions where applicable). This is a plan, not a commitment of dates.

```mermaid
flowchart LR
    M0["M0 · Bootstrap MVP ✅"] --> M1["M1 · Dynamics & stencils ✅"]
    M1 --> M2["M2 · Geometry & I/O ✅"]
    M2 --> M3["M3 · GPU backends 🟡"]
    M3 --> M4["M4 · Multiphysics ✅"]
    M4 --> M5["M5 · Mobile & bindings GA"]
```

Milestones **M0–M2 and M4 are complete**; **M3** is done except SYCL; **M5** (mobile + bindings
GA) is the active frontier, alongside packaging and analysis features called out below.

## M0 — Bootstrap MVP  *(done — Palabos-validated)*

[`bootstrap-cyberfluids-core`](../openspec/changes/archive/2026-07-04-bootstrap-cyberfluids-core):
CMake + NumPP, D2Q9+D3Q19, BGK, collide-and-stream, bounce-back + moving-wall + Zou/He +
periodic, CPU backend, 2D+3D lid-driven cavity, Palabos oracle regression (~0.8% RMS of U),
Python + Swift bindings.

## M1 — Dynamics & stencils  *(done)*

TRT, MRT, regularized, and forced dynamics; D3Q27 and D2Q5/D3Q7 advection-diffusion
descriptors.

## M2 — Geometry & I/O  *(done)*

VTK `STRUCTURED_POINTS` output, checkpoint/restart, and STL/OBJ import +
voxelization via CyberMeshGenerator (optional `CYBERFLUIDS_GEOMETRY` component;
core stays dependency-free). Off-lattice no-slip is realized through partial
bounce-back — the voxelized solid fraction `ns` drives `PorousForcedBGKdynamics`,
so imported geometry becomes walls with no new collision/streaming code.
Validated by voxel-volume convergence, geometry-defined Poiseuille flow, and a
CyberMeshGenerator STL round-trip.

## M3 — GPU backends  *(mostly done)*

Metal (Apple Silicon, D2Q9 cavity), CUDA (NVIDIA), and OpenCL — each behind the backend seam and
validated against the CPU path. CUDA/OpenCL reach ~30× the parallel CPU on an RTX 5060 (see
[benchmarks](benchmarks.md)). **Remaining:** SYCL (AMD/Intel/integrated), and GPU coverage in CI
(currently validated on developer hardware only).

## M4 — Multiphysics  *(done)*

Shan-Chen multi-component/multiphase, porous media (partial bounce-back), thermal flow
(Boussinesq + advection-diffusion coupling).

## M5 — Mobile & bindings GA  *(active)*

iOS/iPadOS and Android NDK toolchain builds, hardened and exercised in CI; Python and Swift
bindings to general availability with full field interop — including a self-contained,
`pip install`-able Python wheel that builds the C ABI (the install/export groundwork is in place).

## Beyond the milestones

- **Analysis:** momentum-exchange **drag/lift** reporting for the wind tunnel (force coefficients).
- **Packaging:** Conan/vcpkg packages for Cyberfluids and NumPP (an installable
  `find_package(Cyberfluids)` already ships; see [getting-started](getting-started.md)).
- **Physics depth:** turbulence models (LES/Smagorinsky) and additional boundary schemes.

See open capabilities in [features](features.md) and the target contract in
[`../openspec/specs/`](../openspec/specs).
