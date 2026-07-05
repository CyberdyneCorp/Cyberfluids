# language-bindings (delta)

## ADDED Requirements

### Requirement: Wind-tunnel binding
The Python binding SHALL expose a 3D wind-tunnel solver for external flow past an
obstacle, so a user can load geometry, run the simulation, and export the flow for
visualization without writing C++.

The binding SHALL allow constructing a tunnel with an analytic sphere obstacle
(available in any build) or from an STL/OBJ mesh (available when the library is
built with geometry support). It SHALL run the simulation, return the velocity and
solid-fraction fields as NumPy arrays, and write a legacy-VTK STRUCTURED_POINTS
file (velocity, speed, solid mask) openable in ParaView. When STL loading is
requested from a build without geometry support, the binding SHALL raise a clear
error directing the user to enable it.

#### Scenario: Sphere wind tunnel from Python
- **WHEN** a `WindTunnel` is created, given a sphere obstacle, and run
- **THEN** `velocity()` SHALL return a finite `(nx, ny, nz, 3)` array with ~zero
  velocity inside the solid and a momentum-deficit wake behind it

#### Scenario: Export for visualization
- **WHEN** `write_vtk(path)` is called after a run
- **THEN** a VTK STRUCTURED_POINTS file with the velocity vector, speed, and solid
  fields SHALL be produced and openable in ParaView

#### Scenario: STL loading requires a geometry build
- **WHEN** `WindTunnel.from_stl` is called on a library built without geometry support
- **THEN** it SHALL raise an error telling the user to rebuild with geometry enabled
