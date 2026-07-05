# Tasks

## 1. Solver
- [x] 1.1 `solver/wind_tunnel3d.hpp`: WindTunnel3D (far-field inlet/walls,
      zero-gradient outlet, obstacle via ns, solid-at-rest seeding)
- [x] 1.2 setObstacleSphere (analytic) + setObstacleField (embed voxel grid) + writeVtk + omegaForReynolds

## 2. C ABI + Python
- [x] 2.1 capi.h/capi.cpp: cf_wind_tunnel_* (always) + create_from_stl / cf_geometry_available (guarded)
- [x] 2.2 CMake: link cyberfluids_geometry + define CYBERFLUIDS_WITH_GEOMETRY when geometry ON
- [x] 2.3 Python WindTunnel class (create/from_stl/set_sphere/run/velocity/solid/write_vtk)

## 3. Example + validation
- [x] 3.1 `examples/wind_tunnel.py` (sphere default, --stl opt-in, VTK + PNG)
- [x] 3.2 `tests/test_wind_tunnel.cpp` (always-on, analytic sphere): stability, no-slip
      inside solid, free-stream upstream, wake deficit, shoulder speed-up
- [x] 3.3 `bindings/python/test_wind_tunnel.py` smoke; register both; full suite green

## 4. Spec & review
- [x] 4.1 language-bindings delta (WindTunnel binding + VTK export)
- [x] 4.2 openspec validate --all --strict
- [x] 4.3 Adversarial review: fixed 5 findings — nx>=3 dim guards (no C-ABI throw/OOB), obstacle-on-face keeps no-slip, create_from_stl dim validation, strengthened test with load-bearing front-stagnation/wake/deflection checks + tighter stability bound
- [x] 4.4 Archive
