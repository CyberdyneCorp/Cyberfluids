# geometry-and-io

## Purpose

Cyberfluids SHALL import complex geometries from STL surfaces, voxelize them into the
lattice, export simulation fields for visualization, and checkpoint/restart long runs.
## Requirements
### Requirement: STL import and voxelization
The library SHALL read triangulated surfaces from STL files and voxelize them into a domain
flag field that classifies each cell as fluid, solid, or boundary.

#### Scenario: Voxelize a geometry
- **WHEN** an STL file is loaded and voxelized at a given resolution into a domain
- **THEN** each cell SHALL be classified as fluid, solid, or boundary consistently with the
  surface, and the classification SHALL drive off-lattice boundary conditions

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

### Requirement: Checkpoint and restart
The library SHALL serialize the full lattice state to disk and restore it, so an
interrupted simulation resumes bit-for-bit (up to floating-point representation) from the
last checkpoint.

#### Scenario: Resume from checkpoint
- **WHEN** a simulation is checkpointed at step N and later restarted from that checkpoint
- **THEN** continuing to step M SHALL yield the same state as an uninterrupted run to step M

