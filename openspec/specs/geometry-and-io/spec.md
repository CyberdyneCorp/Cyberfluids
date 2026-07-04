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
The library SHALL export macroscopic fields (density, velocity, and derived quantities) in
a standard format viewable by external tools (e.g. VTK for Paraview).

#### Scenario: Export a velocity field
- **WHEN** velocity is written for a time step
- **THEN** a VTK-compatible file SHALL be produced that opens in Paraview and shows the
  velocity field on the correct grid

### Requirement: Checkpoint and restart
The library SHALL serialize the full lattice state to disk and restore it, so an
interrupted simulation resumes bit-for-bit (up to floating-point representation) from the
last checkpoint.

#### Scenario: Resume from checkpoint
- **WHEN** a simulation is checkpointed at step N and later restarted from that checkpoint
- **THEN** continuing to step M SHALL yield the same state as an uninterrupted run to step M
