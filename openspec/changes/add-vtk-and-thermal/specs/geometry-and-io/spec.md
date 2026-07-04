## MODIFIED Requirements

### Requirement: Field output for visualization
The library SHALL export macroscopic fields (density, velocity, temperature, and derived
quantities) to a file that ParaView opens directly, using the **legacy VTK `STRUCTURED_POINTS`**
format written by the library itself (no external VTK dependency). The writer SHALL support 2D
and 3D grids, named scalar and vector fields, and SHALL emit points in VTK order (X fastest, then
Y, then Z), writing vectors as three components (with the third component zero in 2D).

#### Scenario: Export a velocity field
- **WHEN** velocity is written for a time step
- **THEN** a VTK `STRUCTURED_POINTS` file SHALL be produced with a valid header (magic, `ASCII`,
  `DATASET STRUCTURED_POINTS`, `DIMENSIONS`, `POINT_DATA`) and the velocity vector at each grid
  point in the correct order, openable in ParaView

#### Scenario: Round-trip fidelity
- **GIVEN** a scalar field with distinct values per cell is written
- **WHEN** the file is parsed back
- **THEN** each point's value SHALL match the field value at the corresponding (x, y, z) under
  the VTK X-fastest ordering
