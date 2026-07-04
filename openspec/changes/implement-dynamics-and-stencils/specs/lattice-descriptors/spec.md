## MODIFIED Requirements

### Requirement: Advection-diffusion descriptors
The library SHALL provide the reduced-velocity **base stencils** for scalar
advection-diffusion — **D2Q5** and **D3Q7** — as compile-time descriptors satisfying the
`LatticeDescriptor` Concept (dimension, velocity set, weights, `cs2`, opposites).

Carrying the advection velocity as a per-cell external field, and the `WithSource` (reaction
term) variants, are deferred until external-field infrastructure is added; the base stencils
are usable on their own.

#### Scenario: Scalar transport stencil
- **WHEN** a lattice is templated on `D2Q5` or `D3Q7`
- **THEN** it SHALL expose the reduced velocity set with the correct weights and `cs2`, and
  SHALL satisfy the `LatticeDescriptor` Concept

#### Scenario: Advection velocity as external field
- **GIVEN** external-field infrastructure is not yet available
- **THEN** the advection-velocity external field and `WithSource` variants SHALL be provided
  by a later change, not this one
