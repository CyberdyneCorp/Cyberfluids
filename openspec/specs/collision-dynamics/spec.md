# collision-dynamics

## Purpose

Cyberfluids SHALL express the physics of each lattice cell through a **Dynamics** object:
the local collision operator, the equilibrium distribution, and how macroscopic moments
are computed from populations. A family of collision models (BGK, TRT, MRT, regularized,
forced) SHALL be selectable per cell or per region without changing surrounding code. The
formulations SHALL match the Palabos reference.

## Requirements

### Requirement: Dynamics interface
The library SHALL define an abstract `Dynamics<T, Descriptor>` interface implementing at
minimum `collide()`, `computeEquilibrium()`, moment computations (`computeDensity`,
`computeVelocity`, `computePiNeq`/stress), relaxation-rate access (`getOmega`/`setOmega`),
and trait predicates (`isBoundary`, `isAdvectionDiffusion`). The relaxation convention
SHALL be `omega = 1/tau`.

#### Scenario: Local collision
- **WHEN** the lattice performs a collision step over a cell with attached Dynamics
- **THEN** `collide()` SHALL relax the cell's populations toward the model's equilibrium
  at rate `omega`

### Requirement: BGK single-relaxation-time model
The library SHALL provide a single-relaxation-time (BGK) collision model constructed from
an `omega`, relaxing toward the second-order (O(Ma²)) equilibrium, plus an incompressible
variant.

#### Scenario: Standard BGK fluid
- **WHEN** cells are assigned `BGKdynamics(omega)` and the simulation runs
- **THEN** each cell SHALL relax toward the second-order equilibrium at rate `omega`, so
  the kinematic viscosity SHALL be `nu = cs2 * (1/omega - 1/2)`

### Requirement: TRT and MRT models
The library SHALL provide two-relaxation-time (TRT) dynamics relaxing symmetric and
antisymmetric population parts at `omegaPlus`/`omegaMinus`, and multiple-relaxation-time
(MRT) dynamics relaxing in moment space via a moment transform.

#### Scenario: TRT stability control
- **WHEN** cells are assigned TRT dynamics with a chosen magic parameter linking
  `omegaMinus` to `omegaPlus`
- **THEN** the antisymmetric relaxation SHALL follow that relation, giving viscosity-
  independent wall placement

#### Scenario: MRT moment-space relaxation
- **WHEN** cells are assigned MRT dynamics
- **THEN** collision SHALL transform populations to moment space, relax each moment at its
  own rate, and transform back

### Requirement: Regularized model
The library SHALL provide regularized LB collision, available both as a decorator wrapping
a base dynamics and as a concrete `RegularizedBGKdynamics`, that projects the
non-equilibrium part before relaxation.

#### Scenario: Regularized BGK
- **WHEN** cells are assigned regularized BGK dynamics and collision runs
- **THEN** the non-equilibrium populations SHALL be regularized (projected onto the
  Hermite/stress basis) before relaxation

### Requirement: Forced dynamics
The library SHALL provide forced collision variants that add a body-force term (e.g.
Guo forcing) using the force stored as a per-cell external field.

#### Scenario: Body force applied
- **WHEN** cells use a forced dynamics on a forced descriptor with a nonzero force field
- **THEN** collision SHALL incorporate the forcing term and the recovered momentum SHALL
  include the half-force correction
