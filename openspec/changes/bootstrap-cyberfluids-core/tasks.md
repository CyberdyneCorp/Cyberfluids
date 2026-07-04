## 1. Build system & dependencies

- [x] 1.1 Create root `CMakeLists.txt` (C++20, CMake ≥ 3.24) and project layout (`include/`, `cmake/`, `tests/`, `scripts/`)
- [x] 1.2 Add `cmake/` dependency module resolving NumPP/SciPP via install-prefix + `find_package(CONFIG)` (SciPP itself consumes NumPP this way, so FetchContent is unworkable); `scripts/bootstrap_deps.sh` builds+installs into `.deps`. NumPP 1.6.0 resolved; SciPP pending its own install/export rules (not needed for the MVP)
- [x] 1.3 Add backend feature flags: `CYBERFLUIDS_CPU` (on), `CYBERFLUIDS_CUDA`/`_METAL`/`_OPENCL`/`_SYCL` (off by default); no MPI dependency
- [ ] 1.4 Add iOS (AppleClang) and Android NDK toolchain configuration for the core
- [ ] 1.5 Wire CTest and a CI-friendly `validate + build + test` target  *(CTest wired; CI target pending)*

## 2. Lattice descriptors

- [x] 2.1 Define the `LatticeDescriptor` C++20 Concept (d, q/numPop, c, cNormSqr, t, cs2, invCs2)
- [x] 2.2 Implement `D2Q9` descriptor with correct velocities, weights, and `cs2=1/3`
- [x] 2.3 Implement `D3Q19` descriptor
- [x] 2.4 Unit-test descriptor constants (weights sum, 1st/2nd moments, opposites, cNormSqr) and add a negative compile test for a non-conforming type

## 3. Core data structures (NumPP-backed)

- [x] 3.1 Implement NumPP-backed SoA population storage (`[q][cells]`) for a domain — `PopulationField<T,Descriptor>` over `numpp::ndarray`; layout verified by test
- [x] 3.2 Implement `ScalarField<T>` and `TensorField<T,N>` over NumPP arrays (2D/3D) — C-order layout verified by test
- [x] 3.3 Implement `BlockLattice<T,Descriptor>` (2D/3D): allocation, `getBoundingBox()`, cell access; `BlockLattice2D`/`3D` aliases
- [x] 3.4 Implement `Cell<T,Descriptor>` as a view: `operator[](iPop)`, dynamics reference *(external-field storage deferred until a forced descriptor lands — no external fields in the MVP stencils)*
- [x] 3.5 Implement `attributeDynamics(subdomain, dynamics)` region assignment
- [x] 3.6 Enforce smart-pointer ownership; unit-test lifetime with no manual `delete` (lattice in `unique_ptr`, dynamics in a `shared_ptr` registry)

## 4. Collision & time step

- [x] 4.1 Define abstract `Dynamics<T,Descriptor>` interface (collide, computeEquilibrium, moments, get/setOmega, traits) — needed by `Cell`
- [x] 4.2 Implement `BGKdynamics(omega)` with second-order equilibrium; verify `nu = cs2*(1/omega - 1/2)`
- [x] 4.3 Implement `collide()`, `streamPeriodic()`, and fused `collideAndStream()` (backend-dispatched over an index range)
- [x] 4.4 Unit-test: fused vs separate identical; equilibrium recovers rho/u; collide conserves mass/momentum

## 5. Boundary conditions

- [x] 5.1 Implement bounce-back no-slip (`boundary::noSlipReflected`) + moving-wall momentum term
- [x] 5.2 Implement Zou/He velocity (Dirichlet) boundary — 2D top face (`boundary::zouHeVelocityTop`), self-consistency tested *(other faces/edges/corners and 3D deferred to a follow-up)*
- [ ] 5.3 Implement `setVelocityConditionOnBlockBoundaries` convenience application *(deferred — the cavity applies the lid velocity directly; generic multi-face applicator is a follow-up)*
- [x] 5.4 Implement periodic boundaries per axis (`streamPeriodic` full wrap; tested for axis + diagonal wrap)
- [x] 5.5 Unit-test boundary behavior: Zou/He recovers imposed u/rho; bounce-back reflection + moving-wall term

## 6. CPU backend

- [x] 6.1 Define the backend abstraction seam (`Backend::forEachIndex(n, f)`; `backend::Default`)
- [x] 6.2 Implement the CPU backend using `std::execution::par_unseq` (guarded by `__cpp_lib_parallel_algorithm`) with a serial fallback
- [x] 6.3 Add GPU backend stubs (CUDA/Metal/OpenCL/SYCL) behind feature-flag defines to lock the seam
- [x] 6.4 Verify collide-and-stream loop body is backend-agnostic (templated on `Backend`; CPU covers all indices — test)

## 7. Lid-driven cavity example (Navier-Stokes)

- [x] 7.1 Build a 2D D2Q9 lid-driven cavity (BGK + bounce-back walls + moving-wall bounce-back lid, chosen over Zou/He for exact mass conservation) — mass conserved to rounding; classic S-curve centerline
- [x] 7.2 Build a 3D D3Q19 lid-driven cavity (BGK + bounce-back walls + moving-wall lid)
- [x] 7.3 Run to steady state and dump centerline velocity profiles (`writeCenterlines` CSV; cavity2d/cavity3d demos)

## 8. Palabos oracle validation

- [x] 8.1 Set up equivalent lid-driven cavity in Palabos (matched N/omega/U/steps); revision `4127697` recorded — `tests/oracle/palabos/cavity2d_oracle.cpp`
- [x] 8.2 Generate/store Palabos reference for the 2D cavity (`tests/oracle/cavity2d_palabos.csv`) *(3D reference deferred with the 3D oracle)*
- [x] 8.3 Implement the in-process comparison harness (L2/L∞ over interior centerlines, normalized by U); no Palabos/MPI in the test build
- [x] 8.4 Add regression test asserting agreement (L∞≤0.07, L2≤0.015 of U); reports observed max deviation + location (observed L∞≈0.050 at y=62, L2≈0.008)
- [x] 8.5 Document parameters, tolerance, Palabos revision, and regeneration steps (`tests/oracle/README.md`)

## 9. Language bindings

- [x] 9.1 Define the thin C-ABI core surface (`include/cyberfluids/capi.h` + `cyberfluids_c` shared lib): cavity create/destroy/run/step, nx/ny, velocity/density readout
- [x] 9.2 Implement the Python binding (ctypes over the C ABI); fields returned as NumPy arrays mirroring the NumPP-backed data
- [x] 9.3 Implement the Swift binding (SwiftPM system-library module wrapping the C ABI)
- [x] 9.4 Add smoke tests (CTest `python_binding` + `swift_binding`): build+run the cavity, read velocity, assert lid-driven flow — both pass

## 10. Validation & docs

- [x] 10.1 `openspec validate --all --strict` passes (12 items) + full test suite 12/12
- [x] 10.2 Update README/docs: real API examples, quick start (bootstrap_deps + ctest), honest feature/backend status, oracle-test instructions, bindings doc
