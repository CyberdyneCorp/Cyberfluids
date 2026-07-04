## MODIFIED Requirements

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
