# Benchmarks

Throughput reference results for the D3Q19 wind-tunnel solver across compute backends. The
metric is **GLUPS** — giga lattice-updates per second (`nx·ny·nz · steps / seconds / 1e9`);
higher is better. These are recorded reference points, not a promise: re-run
[`just bench`](../justfile) on your own hardware to compare.

## Methodology

- **Solver:** the D3Q19 wind tunnel (sphere obstacle, Re = 100) — the CPU `WindTunnel3D`
  (fp64, `std::execution::par_unseq`) versus the CUDA and OpenCL device-kernel solvers (fp32).
- **Harness:** [`bench/wind_tunnel_bench.cpp`](../bench/wind_tunnel_bench.cpp), run via
  `just bench [nx ny nz steps]`. Each backend runs an identical simulation.
- **Timing:** 50 warm-up steps (JIT, allocations, first-touch) are excluded; the timed window
  ends with a `velocity()` read so any pending device→host copy is included, not hidden.
- **Accuracy gate:** the GPU (fp32) fields agree with the fp64 CPU oracle to ~1e-5 (`Linf/Uin`),
  enforced by the `cuda_wind_tunnel` / `opencl_wind_tunnel` CTest cases.

## Results — NVIDIA RTX 5060 + Intel i9-12900K

Host: i9-12900K (24 threads) · RTX 5060 (8 GB, driver 580.95.05 / CUDA 13.0) · nvcc 12.0
(kernels built for `sm_90`; the driver JIT-compiles the embedded PTX onto Blackwell `sm_120`).

| Grid | Steps | Backend | Precision | GLUPS | vs CPU |
|------|-------|---------|-----------|------:|-------:|
| 160×80×80 (1.0M) | 500 | CPU | fp64 | 0.038 | 1.0× |
| 160×80×80 (1.0M) | 500 | CUDA | fp32 | 1.135 | 29.9× |
| 160×80×80 (1.0M) | 500 | OpenCL | fp32 | 1.149 | 30.2× |
| 256×128×128 (4.2M) | 300 | CPU | fp64 | 0.038 | 1.0× |
| 256×128×128 (4.2M) | 300 | CUDA | fp32 | 1.153 | 30.2× |
| 256×128×128 (4.2M) | 300 | OpenCL | fp32 | 1.153 | 30.2× |

## Reading the numbers

- **~30× over a fully parallel CPU.** The CPU baseline is real — it saturates ~22 of the 24
  hardware threads (`par_unseq`), so this is not a serial-vs-parallel strawman.
- **CUDA ≈ OpenCL.** The kernels are line-for-line identical and the LBM step is
  **memory-bandwidth-bound**: at ~1.15 GLUPS the solver moves ~350 GB/s, ≈ 78 % of the RTX
  5060's ~448 GB/s peak. Both APIs hit the same DRAM roofline, so neither wins — and the card
  is being used near-optimally despite the deliberately simple kernel.
- **Precision caveat.** GPU is fp32, CPU is fp64. Halving precision roughly halves memory
  traffic, so a same-precision comparison would narrow the 30× — but fp32 is the conventional,
  accuracy-validated choice for LBM GPU work here.

## Cross-machine reference (indicative)

| Device | Backend | GLUPS | Notes |
|--------|---------|------:|-------|
| Apple M2 Max GPU | OpenCL | ~1.35 | Unified memory (~400 GB/s class); measured separately |
| NVIDIA RTX 5060 | OpenCL / CUDA | ~1.15 | This machine (table above) |

Both are bandwidth-bound and in the same memory-bandwidth class, so the figures land close.
This is **not** a controlled head-to-head (different grids/steps, different measurement runs) —
treat it as indicative, not a ranking.

## Reproducing

```sh
just gpu-detect          # confirm which backends this host has
just bench               # default 160×80×80, 500 steps
just bench 256 128 128 300
```

The benchmark is built behind `-DCYBERFLUIDS_BUILD_BENCH=ON` and links whichever GPU backends
were enabled, so a CPU-only host still builds it and reports a CPU number.
