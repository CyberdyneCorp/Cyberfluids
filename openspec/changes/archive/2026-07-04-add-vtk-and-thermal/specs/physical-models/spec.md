## MODIFIED Requirements

### Requirement: Thermal flow (Boussinesq)
The library SHALL couple a fluid lattice to an advection-diffusion "temperature" lattice with
**two-way coupling**: (1) the fluid velocity advects the temperature (via the external advection
velocity of the AD lattice), and (2) the temperature produces a buoyancy body force
`F = rho·g·β·(T − T_ref)` along the gravity axis, applied to the fluid through its per-cell
external force. Fixed-temperature (Dirichlet) walls SHALL set the hot/cold plates. The kinematic
viscosity and thermal diffusivity SHALL be set through the fluid/temperature relaxation rates.

#### Scenario: Pure conduction (gravity off)
- **GIVEN** hot and cold Dirichlet walls with no gravity
- **WHEN** the coupled system reaches steady state
- **THEN** the temperature SHALL approach the linear conduction profile between the plates and the
  fluid velocity SHALL decay to ~0

#### Scenario: Rayleigh-Bénard convection
- **GIVEN** a hot bottom wall and cold top wall with buoyancy at Rayleigh number Ra
- **WHEN** Ra is below the critical value (≈ 1708) the seeded perturbation SHALL decay (no
  convection); and WHEN Ra is above it the perturbation SHALL grow into steady convection with
  nonzero kinetic energy
