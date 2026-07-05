# Tasks

## 1. Core voxelizer (dependency-free)
- [x] 1.1 `geometry/voxel_field.hpp`: VoxelField (z-fastest SDF grid), Sphere/Slab
      SDFs, voxelizeImplicit, sdfToSolidFraction ramp, stampSolidFraction
- [x] 1.2 Off-lattice no-slip realized through ns + PorousForcedBGKdynamics (reuse)

## 2. CyberMeshGenerator bridge (optional, guarded)
- [x] 2.1 `cmake/CyberfluidsGeometry.cmake`: option CYBERFLUIDS_GEOMETRY (OFF),
      add_subdirectory(CMG) tests/CLI off, cyberfluids_geometry TU
- [x] 2.2 `geometry/cmg_loader.hpp` (CMG-free) + `src/geometry/cmg_loader.cpp`
      (only CMG TU): read_plc + voxelize(SDF) + x-fastest->z-fastest re-index
- [x] 2.3 Wire option + include into top CMakeLists; core stays CMG-free by default

## 3. Validation
- [x] 3.1 `test_voxelize_sphere` (always-on): voxel volume -> (4/3)pi R^3 convergence + ns ramp
- [x] 3.2 `test_geometry_poiseuille` (always-on): ns=1 slabs reproduce the analytic
      parabola AND the hand-coded channel within 3%
- [x] 3.3 `test_cmg_roundtrip` (guarded): unit-cube STL volume within 5%, bunny loads non-empty
- [x] 3.4 Register tests; full suite green; default (geometry OFF) build unaffected

## 4. Spec & review
- [x] 4.1 Modify geometry-and-io "STL import and voxelization" + boundary-conditions
      "Off-lattice (STL) bounce-back" requirements
- [x] 4.2 openspec validate --all --strict
- [x] 4.3 Adversarial review: fixed 6 findings — dims-guard OOB, fragile monotonicity, extracted+unit-tested fromXFastest re-index (transpose now caught), Poiseuille routes through shipped stampSolidFraction
- [x] 4.4 Archive
