# build-and-packaging Specification

## Purpose

Define how Cyberfluids is built, how its NumPP/SciPP dependencies are resolved, how the
optional backends are toggled, and how continuous integration and the throughput benchmark
guard the project. The build SHALL produce the core library and test suite from a single
CMake configuration, keep GPU backends optional, depend on no MPI, and stay green in CI.

## Requirements
### Requirement: CMake build with C++20
The project SHALL build with CMake (≥ 3.24) and require C++20, producing the Cyberfluids
core library and its test suite. The build SHALL succeed on GCC, Clang, and AppleClang.

#### Scenario: Configure and build the core
- **WHEN** the project is configured and built with a C++20-capable compiler
- **THEN** the Cyberfluids core library SHALL compile and the test targets SHALL be
  available to CTest

### Requirement: NumPP and SciPP dependency resolution
The build SHALL obtain NumPP (and, when it exports install rules, SciPP) as build dependencies
via a local install prefix — `scripts/bootstrap_deps.sh` builds and installs them into `.deps/`,
where CMake's `find_package(NumPP CONFIG)` / `find_package(SciPP CONFIG)` resolve them — without
requiring a manual system-wide install and without pulling in generic third-party math
libraries. In production these instead come from a package manager (Conan/vcpkg). SciPP is
optional today (no target links it yet); its absence SHALL NOT break the build.

#### Scenario: Dependencies resolved from the bootstrap prefix
- **WHEN** `scripts/bootstrap_deps.sh` has installed NumPP into `.deps/` and the project is
  configured
- **THEN** `find_package(NumPP CONFIG)` SHALL resolve NumPP from `.deps/`, the NumPP-backed
  targets SHALL build, and no other generic math/array library SHALL be required

#### Scenario: Missing dependency fails clearly, not silently
- **WHEN** the project is configured without NumPP installed under the deps prefix
- **THEN** CMake SHALL report NumPP as not found and skip the NumPP-backed targets rather than
  fetching an unpinned copy from the network

### Requirement: Backend feature flags
The build SHALL enable the CPU backend by default and expose off-by-default flags for the
CUDA, Metal, and OpenCL/SYCL backends. Absence of a GPU toolkit SHALL NOT break the CPU
build, and no MPI dependency SHALL be required.

#### Scenario: CPU-only build on a machine with no GPU toolkit
- **WHEN** the project is built with all GPU flags off (the default)
- **THEN** the CPU backend SHALL build and run, and GPU code SHALL be excluded cleanly

#### Scenario: GPU flag toggles a backend
- **WHEN** a GPU backend flag is enabled and its toolkit is present
- **THEN** that backend SHALL be compiled and selectable at runtime

### Requirement: Cross-platform configuration
The build SHALL support desktop/server targets (x86-64, ARM64) and provide toolchain
configuration for iOS/iPadOS (AppleClang) and Android (NDK) cross-compilation of the core.

#### Scenario: Mobile toolchain configuration available
- **WHEN** the build is configured with an iOS or Android NDK toolchain file
- **THEN** the core library SHALL configure for that target without edits to physics code

### Requirement: Continuous integration
The repository SHALL provide a GitHub Actions workflow that builds the project
and runs the test suite on every push to `main` and every pull request, and that
validates the OpenSpec artifacts.

The Linux job SHALL build with GCC (exercising the `std::execution::par_unseq`
CPU backend, linking libstdc++'s parallel backend with `-Wl,--no-as-needed -ltbb`
so the linker keeps libtbb regardless of link-line order) and run the full CTest
suite, including the oracle tests (which compare against in-repo reference CSVs and
require no external solver at runtime). A macOS job SHALL build with the Metal
backend enabled and run the Swift-binding test (the Metal GPU test is excluded — CI
VMs have no guaranteed GPU). A separate OpenSpec job SHALL run
`openspec validate --all --strict`.

#### Scenario: Pull request gate
- **WHEN** a pull request is opened or updated
- **THEN** CI SHALL build the project on Linux and macOS and run the test suites, and the
  checks SHALL fail if any test fails or if `openspec validate --all --strict` fails

#### Scenario: Superseded runs are cancelled
- **WHEN** a new commit is pushed to a branch with a run already in progress
- **THEN** the in-progress run SHALL be cancelled (concurrency group)

### Requirement: Throughput benchmark with recorded references
The repository SHALL provide a reproducible throughput benchmark that times an identical
wind-tunnel run on the CPU backend and on each enabled GPU backend, reporting GLUPS (giga
lattice-updates per second) and the speed-up over the CPU baseline. The benchmark SHALL be
buildable on a CPU-only host (GPU backends compiled in only when enabled) and SHALL be
runnable via a single command. Recorded reference results (hardware, grid, and measured
GLUPS) SHALL be kept in the docs so future runs can be compared against a known baseline.

#### Scenario: Benchmark runs across available backends
- **WHEN** the benchmark is built with `-DCYBERFLUIDS_BUILD_BENCH=ON` and run (e.g. `just bench`)
- **THEN** it SHALL report GLUPS for the CPU backend and for every GPU backend that was enabled,
  each with its speed-up over the CPU baseline

#### Scenario: Reference results are recorded
- **WHEN** the benchmark is run on a reference machine
- **THEN** the measured GLUPS, the hardware, and the grid/step configuration SHALL be recorded
  in the project docs (`docs/benchmarks.md`) as a baseline for comparison

