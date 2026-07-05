# Tasks

## 1. OpenCL (validated here)
- [x] 1.1 `cmake/CyberfluidsOpenCL.cmake` (option, find OpenCL, cyberfluids_opencl lib)
- [x] 1.2 `WindTunnelOpenCL` — collide + stream_bc kernels, resident buffers, ping-pong, fp64 read-back
- [x] 1.3 `tests/test_opencl_wind_tunnel.cpp`: GPU-vs-CPU (Linf/L2/mean), determinism, no-slip; skips w/o device
- [x] 1.4 Validated on Apple M2 Max: Linf/Uin ~ 1e-5, ~1.35 GLUPS

## 2. CUDA mirror (test on NVIDIA)
- [x] 2.1 `cmake/CyberfluidsCUDA.cmake` (enable_language(CUDA), guarded)
- [x] 2.2 `src/cuda/wind_tunnel_cuda.cu` — line-for-line kernel translation + host class
- [x] 2.3 `tests/test_cuda_wind_tunnel.cpp` (mirror; skips w/o CUDA device)
- [ ] 2.4 Compile + validate on an NVIDIA machine (`-DCYBERFLUIDS_CUDA=ON`)  ← run there

## 3. Docs & spec
- [x] 3.1 Correct `docs/backends.md` (forEachIndex is CPU-only; GPU = device-kernel solvers)
- [x] 3.2 hardware-backends spec delta (seam CPU-only; OpenCL/CUDA wind tunnel)

## 4. Review & archive
- [ ] 4.1 openspec validate --all --strict
- [ ] 4.2 Adversarial review (GPU memory/indexing; CUDA correctness) + fix
- [ ] 4.3 Archive
