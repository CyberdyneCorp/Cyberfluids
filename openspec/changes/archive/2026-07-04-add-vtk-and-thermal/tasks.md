## 1. VTK output

- [x] 1.1 Add `io/vtk_writer.hpp`: `VtkStructuredWriter<Dim>` (named scalar/vector accessors over `Coord=array<int64,Dim>`; legacy STRUCTURED_POINTS ASCII; VTK X-fastest order; 2D→nz=1; vectors 3-comp)
- [x] 1.2 Test `test_vtk_writer`: write an injective scalar `s=x+1000y+1e6z` + a vector in 2D and 3D; re-parse; verify header, DIMENSIONS, POINT_DATA, and per-point values in VTK order

## 2. Scalar Dirichlet wall

- [x] 2.1 Add `boundary::antiBounceBackScalar<T,Descriptor>(post, i, c, phiWall)` to `bounce_back.hpp`

## 3. Buoyancy two-way coupling

- [x] 3.1 Add `timestep/buoyancy.hpp`: `BoussinesqParameters<T>` + `applyBuoyancy<Backend>(fluid, temperature, params)` (mirror `copyVelocityToExternal`: geometry check, null-dynamics skip; write `F[axis]=rho·g·β·(T−T_ref)`)
- [x] 3.2 Test `test_buoyancy`: force-map exactness (F_y, F_x=0) to 1e-12; geometry-mismatch throws; unassigned-cell skip

## 4. Rayleigh–Bénard solver

- [x] 4.1 Add `solver/rayleigh_benard.hpp`: `RayleighBenard2D<Backend,T>` (ForcedD2Q9 fluid + AdvectedD2Q5 temperature; step = couple velocity → apply buoyancy → fluid collide/stream (no-slip y, periodic x) → temperature collide/stream (hot/cold anti-bounce-back y, periodic x))
- [x] 4.2 Add `fromDimensionless(Ra, Pr, U_f, H, nx)` factory + `initConductive()`, `perturbTemperature(amp)`, `avgKineticEnergy()`, `horizontallyAveragedT(y)`, `writeVtk(path)`

## 5. Thermal validation

- [x] 5.1 Test `test_thermal_conduction`: gravity off → temperature matches the analytic linear profile (L2 < 1e-3) and max|u| ≈ 0
- [x] 5.2 Test `test_rayleigh_benard`: sub-critical (Ra≈800) KE decays; super-critical (Ra≈8000) KE grows to nonzero — same box/gravity, differ only in omega

## 6. Integration, example, docs

- [x] 6.1 Add new headers to the umbrella; wire new tests into CTest; add a `rayleigh_benard` example that writes a VTK snapshot
- [x] 6.2 Update `docs/features.md` + README feature rows (geometry-and-io VTK, physical-models thermal)
- [x] 6.3 `openspec validate --all --strict` passes; full suite green
