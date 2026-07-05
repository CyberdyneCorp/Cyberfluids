# Add a 3D wind-tunnel solver, binding, and STL example

## Why

M2 gave the core the ability to import an STL and impose no-slip on arbitrary
geometry, but there was no external-flow solver and none of it was reachable from
Python. Users want to load a mesh and *see the flow* around it. This adds the
external-flow solver, exposes it through the C ABI and Python binding, and ships
a runnable example that writes VTK for ParaView.

## What Changes

- **Solver (core, header-only):** `solver::WindTunnel3D` on `PorousD3Q19` with
  `PorousForcedBGKdynamics` (force=0). Far-field free-stream boundaries (inlet +
  the four transverse walls held at the equilibrium of `u=(Uin,0,0)`), a
  zero-gradient outlet, and the obstacle as an `ns` field (no-slip via `ns=1`).
  Obstacle from an analytic sphere (`setObstacleSphere`, no mesh dep) or an
  embedded voxelized mesh (`setObstacleField`). Solid cells are seeded at rest so
  their bounce-back stays quiescent. `writeVtk` emits velocity/speed/solid.
- **C ABI + Python binding:** `cf_wind_tunnel_*` (always present — the tunnel is
  CMG-free) plus `cf_wind_tunnel_create_from_stl` and `cf_geometry_available`
  (STL loading active only in a `-DCYBERFLUIDS_GEOMETRY=ON` build). Python
  `WindTunnel` class: `WindTunnel(...)`/`from_stl(...)`, `set_sphere`, `run`,
  `velocity()`/`solid()` as NumPy, `write_vtk`, `omega_for_reynolds`.
- **Example:** `examples/wind_tunnel.py` — analytic sphere by default (any build),
  `--stl` for a mesh; writes `wind_tunnel.vtk` (+ a matplotlib mid-plane PNG) with
  ParaView instructions.

## Non-goals

- Turbulence modelling / unsteady vortex shedding at high Re (demo targets steady
  laminar Re ~ up to a few hundred). MRT-3D is noted as a stability upgrade.
- Momentum-exchange drag/lift reporting (a future addition).
- Specular free-slip tunnel walls (far-field Dirichlet is used instead — simpler,
  stable; a free-slip upgrade is possible later).
