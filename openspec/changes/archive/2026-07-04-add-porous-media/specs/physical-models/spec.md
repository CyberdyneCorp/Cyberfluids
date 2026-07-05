# physical-models (delta)

## MODIFIED Requirements

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
