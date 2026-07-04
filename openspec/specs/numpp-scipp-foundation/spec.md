# numpp-scipp-foundation

## Purpose

Cyberfluids SHALL build all numerical data handling and scientific computation on the
CyberdyneCorp C++20 ecosystem — **NumPP** (NumPy port) for multidimensional arrays and
tensors, and **SciPP** (SciPy port) for optimization, advanced linear algebra, and
statistics — instead of generic third-party libraries, so the codebase presents a
single, coherent numerical style.

## Requirements

### Requirement: NumPP as the array/tensor foundation
The library SHALL store all grid-resident simulation state — particle distribution
functions (populations) and macroscopic fields (density, velocity, stress) — in NumPP
multidimensional arrays/tensors, using contiguous Structure-of-Arrays layout suitable
for vectorization.

#### Scenario: Populations stored in a NumPP tensor
- **WHEN** a block lattice with a `DdQq` descriptor is allocated for a domain of `N` cells
- **THEN** its populations SHALL be held in a NumPP tensor whose shape exposes the `q`
  distribution components across the `N` cells contiguously

#### Scenario: Macroscopic field as a NumPP array
- **WHEN** density or velocity is computed over a domain
- **THEN** the result SHALL be returned as a NumPP array of matching spatial shape

### Requirement: SciPP for scientific algorithms
The library SHALL use SciPP for numerical algorithms beyond elementwise array math —
including linear-algebra solves, optimization, quadrature, and statistical reductions —
rather than embedding independent implementations.

#### Scenario: Linear-algebra dependency routed through SciPP
- **WHEN** a model requires a dense/sparse linear solve or eigen-decomposition (e.g. MRT
  moment transforms, calibration)
- **THEN** the computation SHALL be performed via SciPP

### Requirement: No generic third-party math dependencies
The library SHALL NOT introduce generic external math/data libraries (e.g. Eigen, Boost,
xtensor) for core numerics; NumPP and SciPP SHALL be the sole numerical dependencies.

#### Scenario: Dependency audit
- **WHEN** the dependency graph of the core library is inspected
- **THEN** NumPP and SciPP SHALL be the only numerical libraries present, and no other
  generic linear-algebra/array library SHALL appear
