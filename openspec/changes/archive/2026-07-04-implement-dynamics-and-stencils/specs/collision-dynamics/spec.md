## MODIFIED Requirements

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
