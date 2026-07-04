## 1. Build wiring

- [x] 1.1 `cmake/CyberfluidsMetal.cmake`: when `CYBERFLUIDS_METAL AND APPLE AND CYBERFLUIDS_HAVE_NUMPP`, enable OBJCXX, build `cyberfluids_metal` from `src/metal/*.mm`, link `-framework Metal -framework Foundation`, define `CYBERFLUIDS_WITH_METAL`
- [x] 1.2 Include it from the root `CMakeLists.txt`; STATUS message when `CYBERFLUIDS_METAL` is ON but not Apple

## 2. Metal solver

- [x] 2.1 `include/cyberfluids/solver/metal/lid_driven_cavity_metal.hpp`: `MetalLidDrivenCavity2D` (pimpl, guarded by `CYBERFLUIDS_WITH_METAL`) with the `LidDrivenCavity2D` public surface + `downloadPopulations()` + static `metalAvailable()`
- [x] 2.2 `src/metal/lid_driven_cavity_metal.mm`: device/queue, runtime MSL compile, two pipelines, two fp32 shared buffers, initEquilibrium (host), two-encoder step, batched run, host-side macro readback
- [x] 2.3 MSL kernels: `collide_bgk` (in place) and `stream_cavity` (pull; bounce-back walls + moving-wall lid) — faithful ports of bgk.hpp + lid_driven_cavity.hpp streamWithBoundaries

## 3. Validation

- [x] 3.1 Test `test_metal_cavity` (guarded): determinism (two runs bit-identical); CPU(fp64) vs Metal(fp32) cavity steady-state velocity agree within L∞ ≤ ~0.02·U; runtime-skip when no Metal device
- [x] 3.2 CMake wiring for the guarded test; `openspec validate --all --strict` + suite green
- [x] 3.3 Update docs/features + README (Metal backend implemented for the D2Q9 cavity, fp32)
