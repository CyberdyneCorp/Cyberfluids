## ADDED Requirements

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
