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
The library SHALL provide a forced collision variant that adds a **uniform (constant) body
force** to the flow using Guo forcing. The force is a fixed vector supplied to the dynamics
(not a per-cell external field). The macroscopic velocity SHALL include the half-force
correction `u = (sum_i f_i c_i + F/2) / rho`, and the collision SHALL add the Guo source term
`S_i = (1 - omega/2) w_i [ (c_i - u)/cs2 + (c_i . u)/cs2^2 c_i ] . F`.

Per-cell external-field forcing (a force that varies in space, stored on the cell) is deferred
until external-field infrastructure is added.

#### Scenario: Body force applied
- **WHEN** cells use forced BGK dynamics with a nonzero uniform force `F`
- **THEN** collision SHALL incorporate the Guo source term and the recovered momentum SHALL
  include the half-force correction

#### Scenario: Poiseuille channel
- **GIVEN** a channel with no-slip walls and a uniform streamwise body force `F`
- **WHEN** the flow reaches steady state
- **THEN** the velocity profile SHALL match the analytic parabola
  `u(y) = F/(2 rho nu) * y (L - y)` within a documented tolerance

### Requirement: Per-cell forced dynamics
The library SHALL provide a forced BGK collision variant that reads a **per-cell (spatially
varying) body force** from the cell's external field and applies Guo forcing (half-force
velocity correction plus the Guo source term). The pre-existing uniform-force
`ForcedBGKdynamics` SHALL remain available and unchanged. The dynamics SHALL require, at compile
time, that the descriptor declares at least `d` external scalars for the force.

#### Scenario: Uniform per-cell force matches the uniform-force result
- **GIVEN** a per-cell forced dynamics whose external force is set to the same constant value at
  every cell
- **WHEN** a Poiseuille channel runs to steady state
- **THEN** the velocity profile SHALL match the analytic parabola within the documented tolerance
  (equivalent to the uniform `ForcedBGKdynamics`)

#### Scenario: Spatially varying force
- **WHEN** the external force differs between cells
- **THEN** each cell's collision SHALL use its own local force

### Requirement: Advection-diffusion dynamics
The library SHALL provide an advection-diffusion BGK collision for the reduced stencils
(D2Q5/D3Q7) that transports a scalar `phi = sum_i f_i` using a **first-order** equilibrium
`feq_i = t_i phi (1 + invCs2 (c_i . u))`, where the advection velocity `u` is read from the
cell's external field (not from the population first moment). The diffusivity SHALL be
`D = cs2 (1/omega - 1/2)`. It SHALL report `isAdvectionDiffusion() == true`.

#### Scenario: Pure diffusion
- **GIVEN** an AD lattice with zero advection velocity and an initial scalar distribution
- **WHEN** it is advanced
- **THEN** the scalar SHALL diffuse at diffusivity `D` (matching the analytic decay within
  tolerance)

#### Scenario: Advection by a constant velocity
- **GIVEN** an AD lattice with a uniform nonzero external velocity
- **WHEN** it is advanced
- **THEN** the scalar distribution SHALL be transported in the velocity's direction

