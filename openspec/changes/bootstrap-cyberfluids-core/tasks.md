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
- [ ] 4.2 Implement `BGKdynamics(omega)` with second-order equilibrium; verify `nu = cs2*(1/omega - 1/2)`
- [ ] 4.3 Implement `collide()`, `stream()`, and fused `collideAndStream()` over C++20 Ranges
- [ ] 4.4 Unit-test: fused vs separate produce identical fields; equilibrium recovers imposed rho/u

## 5. Boundary conditions

- [ ] 5.1 Implement bounce-back no-slip dynamics
- [ ] 5.2 Implement Zou/He velocity (Dirichlet) boundary for faces/edges/corners
- [ ] 5.3 Implement `setVelocityConditionOnBlockBoundaries` convenience application
- [ ] 5.4 Implement periodic boundaries per axis
- [ ] 5.5 Unit-test boundary mass/momentum behavior on small grids

## 6. CPU backend

- [ ] 6.1 Define the backend abstraction seam (kernel-launch interface, runtime selection)
- [ ] 6.2 Implement the CPU backend using `std::execution::par_unseq` with a serial fallback
- [ ] 6.3 Add GPU backend stubs behind flags (no kernels yet) to lock the seam
- [ ] 6.4 Verify collide-and-stream loop body is backend-agnostic

## 7. Lid-driven cavity example (Navier-Stokes)

- [ ] 7.1 Build a 2D D2Q9 lid-driven cavity (BGK + Zou/He lid + bounce-back walls)
- [ ] 7.2 Build a 3D D3Q19 lid-driven cavity
- [ ] 7.3 Run to steady state and dump centerline velocity profiles

## 8. Palabos oracle validation

- [ ] 8.1 Set up equivalent lid-driven cavity in Palabos; record its revision
- [ ] 8.2 Generate/store Palabos reference fields for the 2D and 3D cavities
- [ ] 8.3 Implement the in-process comparison harness (L2/L∞ over field and centerlines) with documented tolerance
- [ ] 8.4 Add regression tests asserting agreement within tolerance; report observed max deviation
- [ ] 8.5 Document parameters, tolerance, and Palabos revision for reproducibility

## 9. Language bindings

- [ ] 9.1 Define the thin C-ABI core surface (create lattice, set dynamics/BCs, step, read fields)
- [ ] 9.2 Implement the Python binding with NumPP↔NumPy field interop
- [ ] 9.3 Implement the Swift binding (SwiftPM module)
- [ ] 9.4 Add smoke tests: build+run the cavity from Python and from Swift

## 10. Validation & docs

- [ ] 10.1 `openspec validate --all --strict` passes
- [ ] 10.2 Update README/build docs with dependencies, backends, and how to run the oracle tests
