## MODIFIED Requirements

### Requirement: Advection-diffusion descriptors
The library SHALL provide the reduced-velocity **base stencils** for scalar
advection-diffusion — **D2Q5** and **D3Q7** — as compile-time descriptors satisfying the
`LatticeDescriptor` Concept (dimension, velocity set, weights, `cs2`, opposites).

The library SHALL also provide **advected variants** (`AdvectedD2Q5`, `AdvectedD3Q7`) that
declare a per-cell advection-velocity external field via the external-field infrastructure, so
the advection velocity can be supplied per cell (e.g. coupled from a fluid lattice). A base
stencil declares no external field and SHALL add no per-cell overhead. The `WithSource`
(reaction-term) variants remain a future extension.

#### Scenario: Scalar transport stencil
- **WHEN** a lattice is templated on `D2Q5` or `D3Q7`
- **THEN** it SHALL expose the reduced velocity set with the correct weights and `cs2`, and
  SHALL satisfy the `LatticeDescriptor` Concept

#### Scenario: Advected variant carries a per-cell velocity
- **WHEN** a lattice is templated on `AdvectedD2Q5` or `AdvectedD3Q7`
- **THEN** it SHALL satisfy `LatticeDescriptor`, declare `d` external velocity scalars, and its
  cells SHALL expose that per-cell advection velocity via the external field
