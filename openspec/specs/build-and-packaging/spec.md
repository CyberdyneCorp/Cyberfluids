# build-and-packaging Specification

## Purpose
TBD - created by archiving change bootstrap-cyberfluids-core. Update Purpose after archive.
## Requirements
### Requirement: CMake build with C++20
The project SHALL build with CMake (≥ 3.24) and require C++20, producing the Cyberfluids
core library and its test suite. The build SHALL succeed on GCC, Clang, and AppleClang.

#### Scenario: Configure and build the core
- **WHEN** the project is configured and built with a C++20-capable compiler
- **THEN** the Cyberfluids core library SHALL compile and the test targets SHALL be
  available to CTest

### Requirement: NumPP and SciPP dependency fetching
The build SHALL obtain NumPP and SciPP as build dependencies (e.g. via CMake FetchContent
pinned to specific revisions) without requiring a manual system-wide install, and SHALL
NOT pull in generic third-party math libraries.

#### Scenario: Dependencies resolved by the build
- **WHEN** the project is configured on a clean machine with network access
- **THEN** NumPP and SciPP SHALL be fetched and linked automatically, and no other generic
  math/array library SHALL be required

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
CPU backend, linking libstdc++'s parallel backend) and run the full CTest suite,
including the oracle tests (which compare against in-repo reference CSVs and
require no external solver at runtime). A separate OpenSpec job SHALL run
`openspec validate --all --strict`.

#### Scenario: Pull request gate
- **WHEN** a pull request is opened or updated
- **THEN** CI SHALL build the project and run the test suite, and the checks SHALL
  fail if any test fails or if `openspec validate --all --strict` fails

#### Scenario: Superseded runs are cancelled
- **WHEN** a new commit is pushed to a branch with a run already in progress
- **THEN** the in-progress run SHALL be cancelled (concurrency group)

