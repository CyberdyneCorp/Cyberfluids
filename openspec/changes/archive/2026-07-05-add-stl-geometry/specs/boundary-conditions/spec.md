# boundary-conditions (delta)

## MODIFIED Requirements

### Requirement: Off-lattice (STL) bounce-back
The library SHALL apply no-slip boundaries on geometries imported from STL/OBJ
surfaces. No-slip SHALL be realized through partial bounce-back (Walsh grey-LBM):
the voxelized per-cell solid fraction `ns in [0,1]` drives `PorousForcedBGKdynamics`,
where `ns=1` is bit-exact node bounce-back (a solid cell) and `0 < ns < 1` gives a
graded interface at partial cells. Fully-solid cells SHALL enforce no-slip at the
fluid–solid interface, requiring no additional collision or streaming operator.

#### Scenario: No-slip on an imported geometry
- **WHEN** an STL geometry is voxelized into the domain and its solid fraction is
  applied through partial bounce-back
- **THEN** cells classified as solid (`ns=1`) SHALL enforce no-slip at the
  fluid–solid interface

#### Scenario: Geometry-defined walls reproduce Poiseuille flow
- **GIVEN** solid slabs (`ns=1`) stamped at the walls of a forced periodic channel
- **WHEN** the flow reaches steady state
- **THEN** the profile SHALL match the analytic Poiseuille parabola (and a
  hand-coded bounce-back channel of the same width) to within a few percent
