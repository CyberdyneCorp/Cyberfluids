# lattice-descriptors

## Purpose

Cyberfluids SHALL parametrize every LBM model over a compile-time **lattice descriptor**
(a `DdQq` stencil) that fixes the spatial dimension `d`, the number of discrete
velocities `q`, the discrete velocity set, the lattice weights, and the lattice sound
speed. Descriptors decouple collision/streaming code from the specific stencil, and
C++20 Concepts SHALL validate descriptor types at compile time.

## Requirements

### Requirement: Descriptor structure
A descriptor SHALL expose, as compile-time constants, the dimension `d`, the velocity
count `q` (also surfaced as `numPop`), the discrete velocity vectors `c[q][d]`, their
squared norms, the weights `t[q]`, and the sound-speed scalars `cs2` and `invCs2`.

#### Scenario: Query stencil size
- **WHEN** `Descriptor::numPop` is referenced on a lattice templated on `Descriptor`
- **THEN** it SHALL equal the descriptor's velocity count `q`

### Requirement: Descriptor concept validation
The library SHALL define a C++20 Concept that a type must satisfy to be used as a
lattice descriptor, and SHALL reject non-conforming types at compile time.

#### Scenario: Non-conforming descriptor rejected
- **WHEN** a type lacking the required `d`, `q`, `c`, `t`, or `cs2` members is supplied
  as a descriptor
- **THEN** compilation SHALL fail with a Concept constraint error rather than a deep
  template error

### Requirement: Standard hydrodynamic descriptors
The library SHALL provide the standard nearest-neighbour hydrodynamic descriptors
**D2Q9** (2D) and **D3Q19** and **D3Q27** (3D), with correct velocity sets, weights, and
`cs2 = 1/3`.

#### Scenario: 2D simulation
- **WHEN** a fluid simulation is templated on `D2Q9`
- **THEN** it SHALL use a 9-velocity stencil with `d=2, q=9`

#### Scenario: 3D simulation
- **WHEN** a fluid simulation is templated on `D3Q19` or `D3Q27`
- **THEN** it SHALL use the corresponding 3D stencil with `d=3`

### Requirement: Advection-diffusion descriptors
The library SHALL provide reduced-velocity descriptors for scalar advection-diffusion —
**D2Q5** and **D3Q7** — carrying the advection velocity as external state.

#### Scenario: Scalar transport lattice
- **WHEN** a scalar transport lattice is templated on `D2Q5` or `D3Q7`
- **THEN** it SHALL use the reduced stencil and expose the advection velocity as a
  per-cell external field

### Requirement: Descriptor variants with external fields
The library SHALL provide descriptor variants that declare per-cell external fields (e.g.
a body force, or stored macroscopic moments) so forcing schemes and coupled models can
store extra state without changing the core cell layout; a variant with no external field
SHALL add no per-cell overhead.

#### Scenario: Forced descriptor
- **WHEN** a simulation using an external-force scheme is templated on a forced variant
  of a base stencil
- **THEN** each cell SHALL carry the declared force components as external fields
