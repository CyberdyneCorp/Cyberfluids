## Why

Cyberfluids is a greenfield C++20 LBM library. Before any physics can be trusted or any
follow-up capability built, the project needs a working build system wired to the
NumPP/SciPP dependencies and a **numerical oracle** that proves our results match the
Palabos reference. This change stands up that foundation and lands the first end-to-end
result: a 2D and 3D BGK lid-driven cavity that runs on the CPU backend and matches Palabos
within tolerance.

## What Changes

- Create the CMake build (C++20, ≥ 3.24) fetching **NumPP** and **SciPP** as dependencies,
  with the CPU backend as the always-on default and GPU backends behind off-by-default flags.
- Implement the minimal core to realize a runnable solver, drawing on the target specs:
  - Lattice descriptors **D2Q9** and **D3Q19** with C++20 Concept validation
    (`lattice-descriptors`).
  - `BlockLattice` + `Cell` + scalar/tensor fields backed by NumPP tensors
    (`core-data-structures`).
  - **BGK** collision dynamics with `omega = 1/tau` (`collision-dynamics`).
  - Collide-and-stream time step over C++20 Ranges (`streaming-and-timestep`).
  - **Bounce-back** no-slip and **velocity (Zou/He)** boundaries (`boundary-conditions`).
  - **CPU backend** using `std::execution::par_unseq`, no MPI (`hardware-backends`).
  - **Navier-Stokes lid-driven cavity** setup in 2D and 3D (`physical-models`).
- Add the **Palabos oracle test harness**: run identical cavity setups in Palabos and
  Cyberfluids and assert field agreement within a documented tolerance.
- Scaffold **Python** and **Swift** bindings that can construct and run the cavity
  (`language-bindings`).
- This change realizes an MVP subset of the target specs; it does **not** implement
  TRT/MRT, multiphase, thermal, porous media, STL import, GPU backends, or D3Q27 — those
  are follow-up changes.

## Capabilities

### New Capabilities
- `build-and-packaging`: CMake build, C++20 toolchain matrix, NumPP/SciPP dependency
  fetching, backend feature flags, and cross-platform (desktop/mobile) configuration.
- `oracle-validation`: the methodology and harness for using Palabos as a numerical oracle
  to validate Cyberfluids results within a documented tolerance.

### Modified Capabilities
- None. This change realizes (implements) subsets of existing target specs but does not
  change their requirements.

## Impact

- New files: root `CMakeLists.txt`, `cmake/` dependency modules, `include/cyberfluids/`
  and `src/` core, `bindings/python` and `bindings/swift`, `tests/` including the oracle
  harness.
- New build dependencies: NumPP, SciPP (fetched); Palabos as a test-only oracle.
- Establishes the backend abstraction seam (CPU implemented; GPU stubs) that all later
  physics changes build on.
