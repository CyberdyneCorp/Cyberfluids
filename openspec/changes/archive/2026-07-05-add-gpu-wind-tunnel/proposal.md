# GPU-accelerate the wind tunnel (OpenCL + CUDA device kernels)

## Why

Audit finding: the WindTunnel (and every solver through `Backend::forEachIndex`) runs on the
CPU. The GPU "backends" in `gpu_stubs.hpp` merely run the host lambda serially — the seam takes
a host callable and cannot dispatch to a device, so it was never GPU-capable. Only the bespoke
Metal cavity used the GPU. This change delivers real GPU acceleration for the 3D wind tunnel and
corrects the misleading "seam is GPU-ready" story.

## What Changes

- **OpenCL wind tunnel** (`solver::WindTunnelOpenCL`, `cmake/CyberfluidsOpenCL.cmake`): two
  device kernels — porous convex-blend `collide` and `stream_bc` (inlet/wall/outlet/interior) —
  faithful ports of the CPU solver, with populations resident on the GPU (ping-pong buffers) and
  copy-back only for readout. Validated on this machine's **Apple M2 Max GPU** against the fp64
  CPU `WindTunnel3D` oracle: `Linf/Uin ≈ 1e-5`, deterministic, no-slip preserved; ~1.35 GLUPS.
- **CUDA mirror** (`solver::WindTunnelCuda`, `src/cuda/wind_tunnel_cuda.cu`,
  `cmake/CyberfluidsCUDA.cmake`): line-for-line translation of the OpenCL kernels; authored here,
  compiled + validated on an NVIDIA machine (`-DCYBERFLUIDS_CUDA=ON`).
- **Tests**: `opencl_wind_tunnel` (runs here, skips cleanly without a device) and
  `cuda_wind_tunnel` (runs on NVIDIA) — both compare GPU fp32 vs CPU fp64 within tolerance,
  plus determinism and no-slip.
- **Docs/spec honesty**: clarify that `forEachIndex` is CPU-only and GPU = bespoke device-kernel
  solvers; update `docs/backends.md` and the hardware-backends spec.

## Non-goals

- Routing GPU through `forEachIndex` (architecturally impossible for a host lambda).
- A GPU path in the C ABI / Python binding (follow-up; the C++ solver + tests validate the kernels).
- SYCL; generalizing Metal beyond the 2D cavity.

## Notes

- The ~290× figure is vs the *serial* macOS CPU path (AppleClang libc++ lacks `par_unseq`); vs a
  fully parallel CPU the fair speedup is smaller. The GPU throughput (~1.35 GLUPS) is the honest metric.
