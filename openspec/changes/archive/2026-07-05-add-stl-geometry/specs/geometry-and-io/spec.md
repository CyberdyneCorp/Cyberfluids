# geometry-and-io (delta)

## MODIFIED Requirements

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
