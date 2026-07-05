# collision-dynamics (delta)

## MODIFIED Requirements

### Requirement: TRT and MRT models
The library SHALL provide two-relaxation-time (TRT) dynamics relaxing symmetric and
antisymmetric population parts at `omegaPlus`/`omegaMinus`, and multiple-relaxation-time
(MRT) dynamics relaxing in moment space via a moment transform. MRT SHALL be available
for **both D2Q9 and D3Q19**, and a forced (Guo body-force) MRT variant SHALL be provided.

#### Scenario: TRT stability control
- **WHEN** cells are assigned TRT dynamics with a chosen magic parameter linking
  `omegaMinus` to `omegaPlus`
- **THEN** the antisymmetric relaxation SHALL follow that relation, giving viscosity-
  independent wall placement

#### Scenario: MRT moment-space relaxation
- **WHEN** cells are assigned MRT dynamics
- **THEN** collision SHALL transform populations to moment space, relax each moment at its
  own rate, and transform back

#### Scenario: MRT reduces to BGK
- **WHEN** every non-conserved MRT relaxation rate is set equal to `omega`
- **THEN** the collision output SHALL equal the BGK collision output bit-for-bit
  (and the forced MRT variant SHALL equal forced BGK), for both D2Q9 and D3Q19

#### Scenario: D3Q19 MRT viscosity
- **WHEN** a forced D3Q19 channel is driven to steady state with MRT dynamics
- **THEN** the streamwise profile SHALL match the analytic Poiseuille parabola for
  `nu = cs2(1/omega − 1/2)` (the viscosity set by the shear moments)
