# Backends

Cyberfluids targets a **single node** and deliberately avoids MPI. Parallelism is
intra-node: threads, SIMD, and GPU. The fluid physics never changes when the backend does —
see the [hardware-backends spec](../openspec/specs/hardware-backends/spec.md).

## How it actually works (two distinct mechanisms)

There are **two** parallelism mechanisms, and it's important not to conflate them:

1. **The CPU seam** — `Backend::forEachIndex(n, f)` runs a **host C++ lambda** `f` across
   `[0, n)`, lowering to `std::for_each(std::execution::par_unseq, ...)` (with a serial
   fallback). This is a **CPU-only** abstraction: a host closure cannot be marshalled onto a
   GPU, so this seam does **not** dispatch to devices.

2. **GPU device-kernel solvers** — GPU acceleration is a **bespoke solver per API**, with the
   hot loop (collide + stream) rewritten as device kernels (MSL / OpenCL C / CUDA C++), data
   kept resident on the device across steps. Each is validated against the CPU solver as an
   fp64 oracle. The Metal cavity and the OpenCL/CUDA wind tunnel are built this way — they do
   not go through `forEachIndex`.

```mermaid
flowchart LR
    MODEL["Model code"] --> SEAM["forEachIndex seam<br/>(CPU only)"]
    SEAM --> CPU["CPU · par_unseq / serial"]
    GPUK["Device kernels<br/>(collide + stream)"] --> METAL["Metal (MSL)"]
    GPUK --> OCL["OpenCL C"]
    GPUK --> CUDA["CUDA C++"]
    ORACLE["CPU solver = fp64 oracle"] -.validates.-> GPUK
```

## Backend matrix

| Backend | Hardware | Enable flag | Status |
|---|---|---|---|
| **CPU** | x86-64 (AVX-512), ARM64 (Neon) | on by default | ✅ `std::execution::par_unseq` (guarded by `__cpp_lib_parallel_algorithm`) with a serial fallback |
| **OpenCL** | Apple / AMD / Intel / NVIDIA GPUs | `-DCYBERFLUIDS_OPENCL=ON` | ✅ D3Q19 wind tunnel — device kernels validated on Apple M2 Max GPU (Linf/Uin ~ 1e-5 vs the fp64 CPU oracle; ~1.35 GLUPS) |
| **CUDA** | NVIDIA GeForce / Quadro / Tesla | `-DCYBERFLUIDS_CUDA=ON` | 🟡 D3Q19 wind-tunnel kernels written (line-for-line mirror of the OpenCL path); compiled + validated on an NVIDIA machine |
| **Metal** | Apple Silicon (Mac, iPad) | `-DCYBERFLUIDS_METAL=ON` | ✅ D2Q9 BGK cavity (fp32) — Objective-C++ + runtime MSL kernels |
| **SYCL** | AMD, Intel, integrated GPUs | `-DCYBERFLUIDS_SYCL=ON` | 📋 Planned |

> Note: the `Cuda`/`Metal`/`OpenCL`/`Sycl` structs in `backend/gpu_stubs.hpp` are **not** the
> acceleration path — they only satisfy the `forEachIndex` contract by running the lambda on
> the host (serial), so a flag-enabled build still runs. Real acceleration lives in the
> device-kernel solvers above.

## Guarantees

- Building with **no GPU toolkit** present never breaks the CPU build; GPU code is excluded
  cleanly.
- A GPU run and a CPU run of the same simulation agree within a **documented floating-point
  tolerance** (fp32 device vs fp64 CPU oracle).
- No MPI library or launcher is required to build or run.

Status: the **CPU** path is the always-on baseline. **OpenCL** is a real, validated GPU
wind-tunnel solver (measured ~290× vs the serial macOS CPU path on an M2 Max; the fair figure
vs a fully parallel CPU is smaller). **CUDA** mirrors it and is validated on NVIDIA hardware.
**Metal** accelerates the 2D cavity. SYCL is planned.
