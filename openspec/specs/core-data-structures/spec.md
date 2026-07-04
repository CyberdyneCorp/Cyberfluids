# core-data-structures

## Purpose

Cyberfluids SHALL provide the grid-based data structures that hold the state of an LBM
simulation — populations, scalar fields, and tensor fields — in both 2D and 3D. State
SHALL live in contiguous NumPP tensors, ownership SHALL be expressed with smart pointers,
and the same operation code SHALL run over any backend.

## Requirements

### Requirement: Block lattice storing cells
The library SHALL provide a `BlockLattice<T, Descriptor>` (2D and 3D) that owns a grid of
LBM state templated on a numeric type `T` and a lattice `Descriptor`, exposes per-cell
access, its bounding box, and the core operations `collide()`, `stream()`, and
`collideAndStream()`.

#### Scenario: Collide-and-stream step
- **WHEN** `collideAndStream()` is invoked on a block lattice with dynamics attached
- **THEN** every cell SHALL relax toward equilibrium (collision) and the resulting
  populations SHALL propagate to their neighbours (streaming)

#### Scenario: Query extent
- **WHEN** `getBoundingBox()` is called on a block lattice
- **THEN** it SHALL return the index range the lattice spans

### Requirement: Cell view over populations, dynamics, and external fields
A `Cell<T, Descriptor>` SHALL provide access to that cell's `Descriptor::numPop`
distribution functions (via `operator[](iPop)`), a non-owning reference to its `Dynamics`,
and any descriptor-declared external fields. The lattice — not the cell — SHALL own the
underlying storage and dynamics lifetime.

#### Scenario: Population access
- **WHEN** `cell[iPop]` is read or written for `0 <= iPop < numPop`
- **THEN** it SHALL address that cell's `iPop`-th distribution function in the backing
  NumPP tensor

### Requirement: Scalar and tensor fields
The library SHALL provide `ScalarField<T>` and `TensorField<T, N>` (2D and 3D) backed by
NumPP arrays for storing macroscopic quantities (density, velocity, stress) over a domain.

#### Scenario: Store a velocity field
- **WHEN** velocity is computed over a lattice into a `TensorField<T, d>`
- **THEN** each grid point SHALL hold its `d`-component velocity vector contiguously

### Requirement: Attach dynamics to a sub-domain
The library SHALL allow assigning a `Dynamics` to all cells within a specified sub-domain
of a lattice, so different regions can use different collision models without changing the
surrounding code.

#### Scenario: Assign dynamics over a region
- **WHEN** dynamics is attributed over a box sub-domain of a lattice
- **THEN** cells in that sub-domain SHALL use the specified dynamics for collision, and
  cells outside SHALL be unaffected

### Requirement: Smart-pointer ownership
The library SHALL manage heap-allocated lattices, fields, and dynamics through
`std::unique_ptr`/`std::shared_ptr`; core APIs SHALL NOT expose raw owning pointers or
require manual `delete`.

#### Scenario: No manual deallocation
- **WHEN** a lattice or field goes out of scope
- **THEN** its storage SHALL be released automatically without any caller-side `delete`
