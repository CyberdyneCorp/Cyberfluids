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
The library SHALL provide Shan-Chen pseudopotential multiphase. The single-component model SHALL
compute a non-local interaction force `F(x) = -G psi(rho(x)) sum_i w_i psi(rho(x+c_i)) c_i`
(pseudopotential `psi(rho) = rho0 (1 - e^{-rho/rho0})`, weights `w_i = t_i`) and apply it
through the per-cell external body force, producing an equation of state
`p = cs2 rho + (cs2 G / 2) psi^2` whose mechanical instability (`|G| > 4/rho0`) drives
liquid/vapour separation with surface tension.

The library SHALL also provide a two-species (multi-component) model in which each species is a
fluid lattice with its own external force, coupled by a repulsive inter-species pseudopotential
force `F_sigma(x) = -G psi(rho_self(x)) sum_i w_i psi(rho_other(x+c_i)) c_i` (linear `psi = rho`,
`w_i = t_i`, `G > 0`); above a critical `G` the two species are immiscible and de-mix.

#### Scenario: Single-component phase separation
- **GIVEN** a periodic domain seeded near a uniform density with small noise
- **WHEN** `G` is above the critical magnitude (`|G| > 4/rho0`)
- **THEN** the fluid SHALL spontaneously de-mix into high-density (liquid) and low-density
  (vapour) regions; and WHEN `|G|` is below critical it SHALL remain homogeneous

#### Scenario: Surface tension (Laplace law)
- **GIVEN** a liquid droplet of radius R at equilibrium
- **THEN** the pressure jump across the interface SHALL be linear in `1/R`, with slope equal to
  the surface tension

#### Scenario: Two-species immiscible de-mixing
- **GIVEN** two species seeded mixed on a periodic domain
- **WHEN** the repulsive coupling `G` is above critical
- **THEN** the species SHALL segregate into distinct domains (a rising segregation order parameter)
  with each species' mass conserved; below critical they SHALL stay mixed

### Requirement: Porous media (partial bounce-back)
The library SHALL provide partial bounce-back dynamics so flow through
porous/spongy structures can be simulated via a per-cell solid fraction
`ns ∈ [0, 1]`, stored in a generic per-cell external scalar (`ext::Scalar`) on a
porous descriptor variant (e.g. `PorousD2Q9`).

The collision SHALL apply the Walsh, Burr & Holmes (2009) linear blend in its
convex form
`f_i^out = (1 − ns)·f_i^{forced-BGK,out} + ns·f_opposite(i)`,
where `f_i^{forced-BGK,out}` is the ordinary Guo-forced BGK post-collision
population and `f_opposite(i)` is the pre-collision population reflected back
(a bounce-back node). Any body force SHALL be attenuated by `(1 − ns)` so it
vanishes in fully-solid cells.

#### Scenario: Pure-fluid limit
- **WHEN** a cell has `ns = 0`
- **THEN** the collision output SHALL equal the (forced) BGK collision output
  bit-for-bit

#### Scenario: Fully-solid limit
- **WHEN** a cell has `ns = 1`
- **THEN** the collision output SHALL equal the population swap
  `f_i ← f_opposite(i)` (a no-slip node), conserving mass and reversing momentum

#### Scenario: Flow through a porous block
- **WHEN** a region is assigned partial bounce-back with a solid fraction between 0 and 1
- **THEN** the region SHALL resist flow proportionally to its solid fraction

#### Scenario: Darcy's law
- **GIVEN** a fully-periodic box with uniform `ns` driven by a uniform body force `Fx`
- **WHEN** the flow reaches steady state
- **THEN** the mean through-flow (flux) velocity SHALL equal
  `(1 − ns)·Fx / (2·ns)` (rho = 1) — linear in `Fx` (Darcy's law), decreasing
  monotonically with `ns`, with apparent permeability `k = ν·(1 − ns)/(2·ns)`

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

