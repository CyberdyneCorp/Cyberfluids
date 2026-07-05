# geometry-and-io

## Purpose

Cyberfluids SHALL import complex geometries from STL surfaces, voxelize them into the
lattice, export simulation fields for visualization, and checkpoint/restart long runs.
## Requirements
### Requirement: STL import and voxelization
The library SHALL read triangulated surfaces from STL/OBJ/OFF/PLY files and
voxelize them into a regular axis-aligned grid, using CyberMeshGenerator behind
an optional, build-time-guarded component (`CYBERFLUIDS_GEOMETRY`) so the core
library carries no mesh dependency. Voxelization SHALL support an occupancy
(inside/outside) classification and a signed-distance field (negative inside),
and SHALL expose a per-cell solid fraction `ns in [0,1]` derived from the signed
distance (a linear ramp across the surface) for use by partial bounce-back.

The imported grid SHALL be re-indexed into the library's own cell order
(`c = (x*ny+y)*nz+z`) so it maps directly onto a lattice without a transpose. An
analytic voxelizer for implicit primitives (sphere, slab) SHALL be provided in
the core so geometry-driven flow is testable without the mesh dependency.

#### Scenario: Voxelize a geometry
- **WHEN** an STL/OBJ file is loaded and voxelized at a given resolution
- **THEN** each cell SHALL be classified inside/outside (and assigned a solid
  fraction from the signed distance), consistently with the surface, and the
  classification SHALL drive off-lattice boundary conditions

#### Scenario: Voxel volume converges
- **WHEN** a known solid (e.g. a sphere or unit cube) is voxelized at increasing
  resolution
- **THEN** the inside-cell count times the cell volume SHALL converge to the exact
  solid volume

#### Scenario: Import maps onto the lattice without transpose
- **WHEN** a voxelized grid is stamped onto a lattice
- **THEN** cell (x,y,z) of the lattice SHALL correspond to the same physical cell
  of the imported grid (the x-fastest source order re-indexed to z-fastest)

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
The library SHALL serialize the lattice populations (the `{q, ncells}` distribution functions —
the entire live state between steps) to a NumPy `.npy` file and restore them byte-exactly, so an
interrupted simulation resumes bit-for-bit from the last checkpoint. Loading SHALL reject a file
whose shape does not match the target lattice.

#### Scenario: Resume from checkpoint
- **WHEN** a simulation is checkpointed at step N and later restarted from that checkpoint
- **THEN** continuing to step M SHALL yield bit-identical state to an uninterrupted run to step M
  (on a deterministic backend)

#### Scenario: Round-trip fidelity
- **WHEN** populations are saved and then loaded into a matching lattice
- **THEN** the restored populations SHALL be byte-identical to the saved ones

#### Scenario: Shape mismatch rejected
- **WHEN** a checkpoint is loaded into a lattice of different `q` or `ncells`
- **THEN** the load SHALL fail rather than corrupt state

