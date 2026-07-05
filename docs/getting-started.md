# Getting started

The foundational MVP is implemented and Palabos-validated (see the
[feature table](../README.md#-feature-status)).

## Prerequisites

- A **C++20** compiler: GCC, Clang, or AppleClang.
- **CMake ≥ 3.24**.
- **NumPP** — auto-fetched by the build (pinned tag) if not already installed; no manual
  checkout required. For a pinned local install instead, run `scripts/bootstrap_deps.sh`.
- No MPI, and no generic third-party math libraries, are required.

## Build (CPU backend, default)

The simplest path — no dependency setup; the build fetches a pinned NumPP for you:

```bash
git clone https://github.com/CyberdyneCorp/Cyberfluids.git
cd Cyberfluids
cmake -B build -DCMAKE_BUILD_TYPE=Release    # fetches NumPP via FetchContent if not installed
cmake --build build -j
```

Prefer a pinned local install (and required if you want to `install` Cyberfluids as a
`find_package` package — see below)? Bootstrap NumPP into `.deps/` first:

```bash
scripts/bootstrap_deps.sh                     # build + install NumPP into .deps/
cmake -B build -DCMAKE_BUILD_TYPE=Release     # resolves NumPP via find_package(NumPP CONFIG)
```

Disable the fetch fallback with `-DCYBERFLUIDS_FETCH_DEPS=OFF` for offline/vendored builds.
(SciPP consumes NumPP the same way and will be wired in once it ships install/export rules; it
is not required today.)

> **`just` shortcut:** `just bootstrap && just test` does the install + build + test in one go.
> See the [justfile](../justfile) for GPU and benchmark recipes.

## Use Cyberfluids in your project

With NumPP installed (via `scripts/bootstrap_deps.sh`), install Cyberfluids and consume it from
another CMake project with `find_package`:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build --prefix /your/prefix
```

```cmake
# In the downstream project's CMakeLists.txt:
find_package(Cyberfluids REQUIRED)
target_link_libraries(your_app PRIVATE Cyberfluids::core Cyberfluids::c)
```

The installed `CyberfluidsConfig.cmake` re-resolves NumPP transitively, so point
`CMAKE_PREFIX_PATH` at both your install prefix and the NumPP prefix. Install/export is disabled
for fetch-only builds (the fetched NumPP is not an installed package the config could re-resolve).

## Optional GPU backends

The GPU backends are real device-kernel solvers (not stubs). Enable the one your host has —
absence of a GPU toolkit never breaks the CPU build:

```bash
cmake -B build -DCYBERFLUIDS_CUDA=ON      # NVIDIA — validated D3Q19 wind tunnel
cmake -B build -DCYBERFLUIDS_OPENCL=ON    # AMD / Intel / NVIDIA / Apple — validated D3Q19 wind tunnel
cmake -B build -DCYBERFLUIDS_METAL=ON     # Apple Silicon — D2Q9 BGK cavity
```

`just gpu-detect` reports what this host supports. See [backends](backends.md) and
[benchmarks](benchmarks.md).

## Run the tests

```bash
ctest --test-dir build                    # core, cavity, Palabos oracle, GPU, bindings
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
