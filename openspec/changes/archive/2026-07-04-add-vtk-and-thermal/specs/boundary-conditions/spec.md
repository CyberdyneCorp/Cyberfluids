## ADDED Requirements

### Requirement: Scalar Dirichlet (anti-bounce-back)
The library SHALL provide an anti-bounce-back scalar wall for advection-diffusion lattices that
imposes a fixed scalar value `phi_wall` at the wall: the reflected population is
`f_i = -f_opp(i)^post + 2 t_i phi_wall`. This sets hot/cold plate temperatures in thermal flows.

#### Scenario: Fixed-temperature wall
- **GIVEN** an AD lattice with an anti-bounce-back wall imposing `phi_wall`
- **WHEN** the scalar field reaches steady state under pure diffusion
- **THEN** the wall-adjacent scalar SHALL be consistent with `phi_wall` (the half-node wall value),
  yielding the linear conduction profile between two such walls
