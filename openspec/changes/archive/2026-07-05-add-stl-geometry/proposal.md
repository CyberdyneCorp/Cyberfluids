# Complete geometry: STL/OBJ import + voxelization + off-lattice bounce-back

## Why

M2 (Geometry & I/O) has VTK output and checkpoint/restart, but the geometry
half — importing an arbitrary STL/OBJ surface and running flow around/through it —
was blocked on a voxelizer. CyberMeshGenerator (MIT) now provides one, so we can
finish M2 and lift the library from box/channel/cavity domains to arbitrary
geometry.

## What Changes

- **Core (no external dependency):** `cyberfluids::geometry::VoxelField` (a
  signed-distance grid in our z-fastest cell order), analytic implicit surfaces
  (`Sphere`, `Slab`) + `voxelizeImplicit`, an `sdfToSolidFraction` ramp, and
  `stampSolidFraction` that writes the per-cell solid fraction `ns` into a
  lattice's `ext::Scalar` slot.
- **Off-lattice no-slip via partial bounce-back:** a solid fraction `ns` feeds the
  existing `PorousForcedBGKdynamics` (Walsh grey-LBM). `ns=1` is bit-exact node
  bounce-back, so a voxelized geometry becomes no-slip walls with **no new
  collision or streaming code**; the SDF ramp handles partial boundary cells.
- **Optional CyberMeshGenerator bridge:** guarded by `-DCYBERFLUIDS_GEOMETRY=ON`,
  a single compiled TU (`cmg_loader.cpp`) reads STL/OBJ via `cmg::io::read_plc`,
  voxelizes (signed distance), and re-indexes CMG's x-fastest grid into our
  z-fastest `VoxelField`. CMG is confined to that TU; core and all public headers
  stay CMG-free. Integrated via `add_subdirectory` (CMG ships no CMake export).

## Non-goals

- Per-link Bouzidi interpolated bounce-back (a future enhancement; the volumetric
  `ns` ramp is the graded path now).
- A distinct flag-field streaming rule for sharp staircase walls — realized
  instead through `ns` + the porous operator.
- Momentum-exchange drag reporting (the sphere-drag oracle) — deferred; validation
  uses self-contained analytic oracles.
- Native CAD (STEP/IGES) exact voxelization — a later CyberCadKernel path.

## Assumptions

- CyberMeshGenerator is available as a source checkout (default sibling
  `../CyberMeshGenerator`, overridable via `CYBERFLUIDS_CMG_DIR`).
