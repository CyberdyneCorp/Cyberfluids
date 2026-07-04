# boundary-conditions

## Purpose

Cyberfluids SHALL provide boundary conditions that impose velocity, pressure/density, or
zero-gradient constraints on the lattice — grid-aligned (on-lattice) BCs for box-shaped
domains and off-lattice BCs for arbitrary geometries described by triangulated (STL)
surfaces. Bounce-back (no-slip) and periodic boundaries SHALL be supported. Schemes SHALL
match the Palabos reference.
## Requirements
### Requirement: Bounce-back no-slip
The library SHALL provide full bounce-back no-slip dynamics that reflect incoming
populations back along their opposite directions, plus a momentum-exchange variant that
reports the force on the solid nodes.

#### Scenario: No-slip wall
- **WHEN** cells are assigned bounce-back and streaming occurs
- **THEN** each population SHALL be reflected back along its opposite direction, enforcing
  zero fluid velocity at the wall

#### Scenario: Force on an obstacle
- **WHEN** momentum-exchange bounce-back is used on a solid body
- **THEN** the net force/drag on the body SHALL be computed from the exchanged momentum

### Requirement: Velocity and pressure boundaries
The library SHALL provide grid-aligned velocity (Dirichlet) and pressure/density boundary
conditions, including a Zou/He scheme, applicable per face, edge (3D), and corner, with a
convenience call to set a condition on all outer boundaries of a box lattice.

#### Scenario: Velocity boundary on all walls
- **WHEN** a velocity boundary condition is applied to all outer boundaries of a box lattice
- **THEN** prescribed-velocity constraints SHALL be instantiated on every outer face, edge,
  and corner

#### Scenario: Boundary type selection
- **WHEN** a boundary is applied with a chosen type (`dirichlet`, `neumann`, `freeslip`, or
  `density`)
- **THEN** the imposed constraint SHALL be, respectively, prescribed velocity; zero-gradient;
  zero normal velocity with free tangential slip; or fixed density with Neumann tangential

### Requirement: Periodic boundaries
The library SHALL support periodic boundaries per axis, wrapping streaming across the
opposite domain face.

#### Scenario: Periodic channel
- **WHEN** an axis is marked periodic and streaming runs
- **THEN** populations leaving one face SHALL re-enter at the opposite face on that axis

### Requirement: Off-lattice (STL) bounce-back
The library SHALL apply bounce-back boundaries on geometries imported from STL surfaces,
using the voxelized flag field to distinguish fluid, solid, and boundary cells.

#### Scenario: No-slip on an imported geometry
- **WHEN** an STL geometry is voxelized into the domain and a no-slip off-lattice BC is applied
- **THEN** cells classified as solid SHALL enforce no-slip at the fluid–solid interface

### Requirement: Scalar Dirichlet (anti-bounce-back)
The library SHALL provide an anti-bounce-back scalar wall for advection-diffusion lattices that
imposes a fixed scalar value `phi_wall` at the wall: the reflected population is
`f_i = -f_opp(i)^post + 2 t_i phi_wall`. This sets hot/cold plate temperatures in thermal flows.

#### Scenario: Fixed-temperature wall
- **GIVEN** an AD lattice with an anti-bounce-back wall imposing `phi_wall`
- **WHEN** the scalar field reaches steady state under pure diffusion
- **THEN** the wall-adjacent scalar SHALL be consistent with `phi_wall` (the half-node wall value),
  yielding the linear conduction profile between two such walls

