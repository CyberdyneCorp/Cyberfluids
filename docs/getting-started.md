# Getting started

The foundational MVP is implemented and Palabos-validated (see the
[feature table](../README.md#-feature-status)).

## Prerequisites

- A **C++20** compiler: GCC, Clang, or AppleClang.
- **CMake ≥ 3.24**.
- A **NumPP** checkout beside this repo (or pass its path to the bootstrap script).
- No MPI, and no generic third-party math libraries, are required.

## Build (CPU backend, default)

```bash
git clone https://github.com/CyberdyneCorp/Cyberfluids.git
cd Cyberfluids
scripts/bootstrap_deps.sh                    # build + install NumPP into .deps/
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

`scripts/bootstrap_deps.sh` builds and installs [NumPP](https://github.com/CyberdyneCorp/NumPP)
into `.deps/`, where CMake's `find_package(NumPP CONFIG)` resolves it. (SciPP consumes NumPP
the same way and will be wired in once it ships install/export rules; it is not required for
the MVP.)

## Optional GPU backends

The flags exist and gate compilation; the GPU backends are currently **stubs** (the seam is
locked, no device kernels yet). The CPU backend is the working path.

```bash
cmake -B build -DCYBERFLUIDS_CUDA=ON      # NVIDIA
cmake -B build -DCYBERFLUIDS_METAL=ON     # Apple Silicon
cmake -B build -DCYBERFLUIDS_OPENCL=ON    # AMD / Intel / integrated (OpenCL or SYCL)
```

Absence of a GPU toolkit never breaks the CPU build. See [backends](backends.md).

## Run the tests

```bash
ctest --test-dir build                    # 12 tests: core, cavity, Palabos oracle, bindings
ctest --test-dir build -R oracle_cavity --output-on-failure
```

The oracle regression runs the Cyberfluids cavity and compares its steady-state centerlines
against a stored Palabos reference within a documented tolerance. See
[oracle validation](oracle-validation.md) and [`tests/oracle/README.md`](../tests/oracle/README.md).

## Run the demos

```bash
./build/examples/cavity2d 128 20000       # writes cavity2d_centerlines.csv
./build/examples/cavity3d 48 5000
```

## Bindings

- **Python:** `import cyberfluids` (`PYTHONPATH=bindings/python`, or `pip install bindings/python`)
  — fields return as NumPy arrays.
- **Swift:** `swift run --package-path bindings/swift … cavity-demo` (see
  [`bindings/README.md`](../bindings/README.md) for flags).

## Mobile targets (planned)

Cross-compilation toolchain configs for iOS/iPadOS (AppleClang) and Android (NDK) are a
planned follow-up; the core carries no desktop-only assumptions. See the
[platform-support spec](../openspec/specs/platform-support/spec.md).
