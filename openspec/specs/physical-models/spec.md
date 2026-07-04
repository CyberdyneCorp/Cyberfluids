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
The library SHALL couple a fluid lattice to an advection-diffusion temperature lattice via
a Boussinesq processor that adds buoyancy proportional to local temperature deviation and
advects temperature with the fluid velocity.

#### Scenario: Rayleigh-Bénard convection
- **WHEN** a fluid lattice and a temperature lattice are coupled with gravity, reference
  temperature, and a temperature difference across the domain
- **THEN** buoyancy-driven convection cells SHALL develop and temperature SHALL be
  transported by the flow
