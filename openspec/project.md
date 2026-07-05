# Project Context — Cyberfluids

## Purpose

Cyberfluids is a next-generation **Computational Fluid Dynamics (CFD)** library based
on the **Lattice Boltzmann Method (LBM)**. It is written in **pure, zero-legacy C++20**
and targets a single node — from supercomputers and desktop GPUs down to mobile devices
(iOS/iPadOS and Android) — deliberately **without MPI**.

Cyberfluids uses the **Palabos** library (https://palabos.unige.ch/ —
https://gitlab.com/unigespc/palabos, local checkout at `/Users/leonardoaraujo/work/palabos`)
as its **scientific and numerical reference**: the physics (collision models,
equilibria, boundary schemes) mirror Palabos's validated formulations, and Palabos is
used as a **numerical oracle** in tests. Cyberfluids does **not** reuse any Palabos
source code — it is a clean-room reimplementation.

## Tech Stack

- **Language:** C++20 only. Modern idioms throughout — Concepts, Ranges, smart pointers,
  contiguous data. No raw owning pointers, no legacy macros, no Palabos-inherited code.
- **Math / data foundation (CyberdyneCorp ecosystem):**
  - **NumPP** (https://github.com/CyberdyneCorp/NumPP) — C++20 port of NumPy;
    multidimensional arrays/tensors hold LBM state (populations, macroscopic fields).
  - **SciPP** (https://github.com/CyberdyneCorp/SciPP) — C++20 port of SciPy;
    optimization, advanced linear algebra, statistics.
  - No generic third-party math libraries — all numerical data handling flows through
    NumPP/SciPP for a single, coherent style.
- **Build:** CMake (≥ 3.24). NumPP resolved via `find_package(NumPP CONFIG)` from a local
  install prefix (`scripts/bootstrap_deps.sh` → `.deps/`), with a pinned FetchContent fallback
  when it is not installed. Cyberfluids itself installs a `find_package(Cyberfluids)` package.
- **Compilers:** GCC, Clang, AppleClang (C++20). Clang covers Apple Silicon and Android NDK.
- **Parallelism (single node, no MPI):**
  - CPU: C++17/20 STL parallel algorithms (`std::execution::par_unseq`), auto-vectorized
    (AVX-512, ARM Neon).
  - GPU (optional): NVIDIA **CUDA**, Apple **Metal** (metal-cpp), **OpenCL** / **SYCL**.
- **Language bindings:** **Python** (NumPP ↔ NumPy interop) and **Swift** (iOS/macOS).

## Target platforms

- Desktop/server CPU (x86-64 AVX-512, ARM64 Neon).
- Desktop GPU: NVIDIA (CUDA), AMD/Intel/integrated (OpenCL/SYCL), Apple (Metal).
- Mobile: iOS/iPadOS (Apple Silicon M-series, A-series via native Clang), Android (NDK, ARM).

## Planned repository layout

| Directory | Responsibility |
|-----------|----------------|
| `include/cyberfluids/` | Public C++20 headers |
| `src/core/` | Lattice fields, cells, descriptors, dynamics interface |
| `src/dynamics/` | Collision models (BGK/TRT/MRT, regularized, forced) |
| `src/boundary/` | Bounce-back, velocity/pressure, off-lattice (STL) BCs |
| `src/backends/` | Hardware backend abstraction + CPU/CUDA/Metal/OpenCL/SYCL drivers |
| `src/physics/` | Physical models (Navier-Stokes, Shan-Chen, thermal, porous) |
| `src/io/` | STL import, voxelization, VTK output, checkpointing |
| `bindings/python/` | Python binding |
| `bindings/swift/` | Swift binding |
| `tests/` | Unit tests + Palabos-oracle regression tests |

## Conventions

- **2D/3D duality:** capabilities are provided for both 2D and 3D; specs describe both
  unless behavior differs.
- **Genericity:** lattices and dynamics are templated on a floating type `T` and a
  compile-time lattice `Descriptor`, constrained by C++20 Concepts.
- **Lattice units:** the solver works in dimensionless lattice units; unit conversion is
  a helper responsibility. Relaxation convention is `omega = 1/tau`.
- **Memory:** contiguous Structure-of-Arrays storage in NumPP tensors; ownership via
  `std::unique_ptr`/`std::shared_ptr`; no raw `new`/`delete`.
- **Hardware abstraction:** fluid physics is fully decoupled from the hardware driver;
  switching backends does not change the model equations.

## Testing

- Unit tests for descriptors, moments, collision, streaming, boundaries.
- **Palabos oracle:** identical setups (geometry, Reynolds number, omega, steps) are run
  in Palabos and Cyberfluids; macroscopic fields (density, velocity) must match within a
  documented tolerance. See the `oracle-validation` capability.

## OpenSpec adoption note

Cyberfluids is **greenfield**. The specs under `openspec/specs/` describe the **target
contract** for the library (what it SHALL do when built), decomposed by capability and
informed by the Palabos reference. They are forward-looking, not reverse-engineered from
existing code. Implementation proceeds incrementally through OpenSpec changes; the first
is `bootstrap-cyberfluids-core`. As changes archive, their spec deltas merge into this
baseline.
