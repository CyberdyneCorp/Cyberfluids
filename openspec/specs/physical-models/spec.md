# physical-models

## Purpose

On top of the collision/streaming core, Cyberfluids SHALL provide ready-made physical
models for common fluid regimes — non-thermal Navier-Stokes, multi-component/multi-phase
flow, porous media, and thermal flow — with formulations validated against the Palabos
reference.
## Requirements
### Requirement: Non-thermal Navier-Stokes
The library SHALL simulate incompressible/weakly-compressible non-thermal Navier-Stokes
flow using a single fluid lattice with a hydrodynamic collision model (BGK/TRT/MRT) and
standard boundary conditions, with kinematic viscosity set through `omega`.

#### Scenario: Lid-driven cavity
- **WHEN** a fluid lattice with BGK dynamics and velocity boundaries is advanced to steady
  state
- **THEN** it SHALL converge to the Navier-Stokes solution at the Reynolds number implied
  by `omega` and the lid velocity

### Requirement: Multi-component / multi-phase fluids
The library SHALL provide Shan-Chen pseudopotential coupling (single- and multi-component)
with selectable interparticle potential functions, enabling immiscible fluids (e.g.
oil/water) to separate under the interparticle force.

#### Scenario: Two immiscible fluids separate
- **WHEN** two components are coupled by a Shan-Chen interaction above the critical
  interaction strength
- **THEN** the components SHALL de-mix and form an interface with surface tension

### Requirement: Porous media (partial bounce-back)
The library SHALL provide partial (partially-saturated) bounce-back dynamics so flow
through porous/spongy structures can be simulated via a per-cell solid fraction.

#### Scenario: Flow through a porous block
- **WHEN** a region is assigned partial bounce-back with a solid fraction between 0 and 1
- **THEN** the region SHALL resist flow proportionally to its solid fraction

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

