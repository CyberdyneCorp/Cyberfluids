# Changelog

All notable changes to Cyberfluids are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project aims to follow
[Semantic Versioning](https://semver.org/spec/v2.0.0.html) (pre-1.0: minor/patch may still
carry breaking changes while the API stabilizes).

## [Unreleased]

### Added
- **Self-contained Python wheel** — `pip install ./bindings/python` builds the C ABI via
  scikit-build-core + CMake (NumPP fetched and statically linked into a single bundled
  `libcyberfluids_c`), so the wheel imports with no manual C++ build. `__init__.py` loads the
  bundled library from the installed package.
- **Installable package** — `install`/`export` rules and a generated `CyberfluidsConfig.cmake`
  so downstream projects can `find_package(Cyberfluids)` and link `Cyberfluids::core` /
  `Cyberfluids::c`. The C ABI shared library carries a semver `SOVERSION`.
- **Zero-setup dependency resolution** — NumPP is fetched at a pinned tag via FetchContent when
  it is not already installed, so a plain `cmake -B build` works with no manual checkout
  (`-DCYBERFLUIDS_FETCH_DEPS=OFF` to disable).
- **Benchmark reference** — `docs/benchmarks.md` records methodology and measured GLUPS across
  CPU / CUDA / OpenCL; the `bench/wind_tunnel_bench.cpp` harness runs via `just bench`.
- **Project governance** — `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, this
  changelog, and GitHub issue/PR templates.

- **Consumability spec** — a portable OpenSpec "usable by others" readiness rubric
  (`openspec/specs/consumability`) consolidating the install/packaging/CI/versioning/governance
  requirements, reusable as an adoption checklist for other projects.

### Changed
- Documentation and OpenSpec specs brought back in sync with the shipped code (GPU backends
  marked validated, dependency mechanism corrected, spec `Purpose` placeholders filled).

## [0.0.1] - 2026-07-05

Initial tagged release — a zero-legacy C++20 Lattice Boltzmann CFD engine on the CyberdyneCorp
scientific suite (NumPP), single-node, no MPI.

### Added
- **Physics** — BGK / TRT / MRT / regularized / forced dynamics; D2Q9, D2Q5, D3Q7, D3Q19, D3Q27
  stencils (2D and 3D); single- and multi-component Shan-Chen multiphase; Boussinesq thermal
  (Rayleigh-Bénard); porous media (partial bounce-back); per-cell external forcing and
  advection-diffusion coupling.
- **Geometry & wind tunnel** — STL/OBJ import + voxelization (CyberMeshGenerator); a 3D D3Q19
  wind tunnel with off-lattice bounce-back (analytic sphere or imported mesh).
- **Backends** — CPU (`std::execution::par_unseq` + serial fallback); validated CUDA and OpenCL
  D3Q19 wind-tunnel device kernels (~30× the parallel CPU on an RTX 5060); Metal D2Q9 cavity.
- **Validation** — Palabos numerical oracle for the 2D and 3D lid-driven cavity; fp32 GPU
  agreement with the fp64 CPU oracle to ~1e-5.
- **Bindings & I/O** — Python (NumPy / DLPack) and Swift bindings over a stable C ABI;
  legacy-VTK structured-points output; checkpoint/restart.
- **Tooling** — a `justfile` (build, test, GPU, benchmark, spec), a GitHub Actions CI pipeline
  (Linux GCC + macOS Metal/Swift + OpenSpec validation), and in-repo OpenSpec specs.

[Unreleased]: https://github.com/CyberdyneCorp/Cyberfluids/compare/v0.0.1...HEAD
[0.0.1]: https://github.com/CyberdyneCorp/Cyberfluids/releases/tag/v0.0.1
