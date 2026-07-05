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
The build SHALL resolve NumPP (and, when it exports install rules, SciPP) without requiring a
manual system-wide install and without pulling in generic third-party math libraries. It SHALL
prefer an installed CONFIG package — `scripts/bootstrap_deps.sh` builds and installs them into
`.deps/`, where `find_package(NumPP CONFIG)` / `find_package(SciPP CONFIG)` resolve them — and,
when NumPP is not installed, SHALL fall back to fetching it from a **pinned** git tag via
FetchContent so a plain `cmake -B build` works with no manual sibling checkout. The fetch
fallback SHALL be disableable (`-DCYBERFLUIDS_FETCH_DEPS=OFF`) for offline/vendored builds. In
production these instead come from a package manager (Conan/vcpkg). SciPP is optional today (no
target links it yet); its absence SHALL NOT break the build.

#### Scenario: Dependencies resolved from the bootstrap prefix
- **WHEN** `scripts/bootstrap_deps.sh` has installed NumPP into `.deps/` and the project is
  configured
- **THEN** `find_package(NumPP CONFIG)` SHALL resolve NumPP from `.deps/`, the NumPP-backed
  targets SHALL build, and no other generic math/array library SHALL be required

#### Scenario: Zero-setup build via the pinned fetch fallback
- **WHEN** the project is configured with no installed NumPP and the fetch fallback enabled
  (the default)
- **THEN** CMake SHALL fetch NumPP at the pinned tag via FetchContent and build the NumPP-backed
  targets in-tree, without requiring a manual NumPP checkout

#### Scenario: Fetch fallback disabled
- **WHEN** the project is configured with `-DCYBERFLUIDS_FETCH_DEPS=OFF` and no installed NumPP
- **THEN** CMake SHALL report NumPP as not found and skip the NumPP-backed targets rather than
  fetching from the network

### Requirement: Installable find_package(Cyberfluids) package
When NumPP is available as an installed CONFIG package, the build SHALL install Cyberfluids as a
consumable package: the public headers, the C ABI shared library (with a semver `SOVERSION`), and
a `CyberfluidsConfig.cmake` + version file that export namespaced targets (`Cyberfluids::core`,
`Cyberfluids::c`) and re-resolve NumPP via `find_dependency`. Because the install re-resolves
NumPP as a package, install/export SHALL be disabled when NumPP came from the FetchContent
fallback (a build-only mode). Install/export SHALL be toggleable via `-DCYBERFLUIDS_INSTALL`.

#### Scenario: Downstream project consumes the installed package
- **WHEN** Cyberfluids is installed to a prefix and a separate CMake project calls
  `find_package(Cyberfluids)` and links `Cyberfluids::core` / `Cyberfluids::c`
- **THEN** the config SHALL be found, NumPP SHALL be re-resolved transitively, and the downstream
  project SHALL configure, build, and run against the installed headers and library

#### Scenario: Install skipped for a fetch-only build
- **WHEN** the project is built with NumPP obtained via the FetchContent fallback
- **THEN** the install/export rules SHALL be skipped with a clear message, since the fetched
  NumPP is not an installed package the exported config could re-resolve

### Requirement: Self-contained Python wheel
The Python binding SHALL be installable with `pip install` such that the native C ABI is built
and bundled automatically — no separate manual C++ build. The build backend (scikit-build-core)
SHALL drive the CMake build, resolve NumPP without manual setup, and statically link it into a
single `libcyberfluids_c` shared library placed inside the installed package, so the wheel has no
external `libnumpp` runtime dependency. The binding SHALL load the bundled library from the
installed package, and `$CYBERFLUIDS_LIBRARY` SHALL still override it with an explicit path.

#### Scenario: pip install builds and bundles the native library
- **WHEN** `pip install ./bindings/python` is run in a clean environment with a C++20 toolchain
- **THEN** the C ABI SHALL be compiled with NumPP statically linked, bundled into the installed
  `cyberfluids` package, and `import cyberfluids` SHALL load it and run a simulation without any
  manually pre-built library

#### Scenario: Explicit library override
- **WHEN** `$CYBERFLUIDS_LIBRARY` points to a prebuilt `libcyberfluids_c`
- **THEN** the binding SHALL load that library in preference to the bundled one

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

