# Getting started

> Preview — the build described here is delivered by the
> [`bootstrap-cyberfluids-core`](../openspec/changes/bootstrap-cyberfluids-core) change and
> is not runnable yet.

## Prerequisites

- A **C++20** compiler: GCC, Clang, or AppleClang.
- **CMake ≥ 3.24**.
- Network access on first configure (NumPP and SciPP are fetched automatically).
- No MPI, and no generic third-party math libraries, are required.

## Build (CPU backend, default)

```bash
git clone https://github.com/CyberdyneCorp/Cyberfluids.git
cd Cyberfluids
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

CMake fetches [NumPP](https://github.com/CyberdyneCorp/NumPP) and
[SciPP](https://github.com/CyberdyneCorp/SciPP) at pinned revisions via `FetchContent`.

## Optional GPU backends

Off by default. Enable one when its toolkit is present:

```bash
cmake -B build -DCYBERFLUIDS_CUDA=ON      # NVIDIA
cmake -B build -DCYBERFLUIDS_METAL=ON     # Apple Silicon
cmake -B build -DCYBERFLUIDS_OPENCL=ON    # AMD / Intel / integrated (OpenCL or SYCL)
```

Absence of a GPU toolkit never breaks the CPU build. See [backends](backends.md).

## Mobile targets

Cross-compile the core with a toolchain file:

- **iOS / iPadOS** — AppleClang toolchain (Apple Silicon M-series and A-series).
- **Android** — the Android NDK toolchain for the target ARM ABI.

See [platform-support spec](../openspec/specs/platform-support/spec.md).

## Run the tests (Palabos oracle)

```bash
ctest --test-dir build
```

Regression tests run identical setups in Cyberfluids and Palabos and compare macroscopic
fields within a documented tolerance. See [oracle validation](oracle-validation.md).

## Bindings

- **Python:** `pip install .` (from `bindings/python`) — fields interoperate with NumPy.
- **Swift:** add the SwiftPM module from `bindings/swift`.

See the [language-bindings spec](../openspec/specs/language-bindings/spec.md).
