## ADDED Requirements

### Requirement: Two-way buoyancy coupling
The library SHALL provide a backend-dispatched operation that writes a temperature-derived
buoyancy force into a fluid lattice's per-cell external force field, `F = rho·g·β·(T − T_ref)`
along a configurable gravity axis (other components zero), where `T` is the AD scalar at the
matching cell. It SHALL share the geometry-mismatch and unassigned-cell handling of the one-way
coupling, and SHALL leave the one-way `copyVelocityToExternal` unchanged.

#### Scenario: Buoyancy force written from temperature
- **WHEN** the coupling runs over a fluid lattice and a matching temperature lattice
- **THEN** each fluid cell's external force along the gravity axis SHALL equal
  `rho·g·β·(T − T_ref)` at that cell, and the other force components SHALL be zero

#### Scenario: Geometry mismatch is rejected
- **WHEN** the coupling is invoked on lattices with differing extents
- **THEN** it SHALL fail rather than read or write out of bounds
