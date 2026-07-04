## Context

Designed via a 3-topic design workflow (VTK / Boussinesq coupling / validation strategy). All the
building blocks exist: AD dynamics, per-cell forcing (ExternalForceBGKdynamics on ForcedD2Q9),
external fields, and one-way `copyVelocityToExternal`. This change adds output + the buoyancy
feedback + a solver, reusing the `Backend::forEachIndex` seam throughout.

## Goals / Non-Goals

**Goals:** ParaView-openable field output with no deps; a validated Boussinesq thermal model with
two-way coupling; cheap-but-rigorous validation (analytic conduction + convection onset).
**Non-Goals:** binary/XML VTK; 3D Rayleigh–Bénard; high-Ra turbulence; precise Nusselt numbers.

## Decisions

- **VTK: legacy `STRUCTURED_POINTS`, ASCII.** Zero deps, no base64/zlib. `VtkStructuredWriter<Dim>`
  collects named scalar/vector accessors (`std::function<double(Coord)>` /
  `std::function<std::array<double,Dim>(Coord)>`, `Coord = std::array<int64,Dim>`), and writes in
  **VTK order** (loop z-outer, y, x-inner; point index `p = x + nx*(y + ny*z)`). 2D → `nz=1`;
  vectors emit 3 components (vz=0 in 2D). Header lines exactly: magic, title, `ASCII`,
  `DATASET STRUCTURED_POINTS`, `DIMENSIONS`, `ORIGIN`, `SPACING`, `POINT_DATA`, then `SCALARS`/
  `VECTORS` blocks. Binary (big-endian) is a noted follow-up.
- **Scalar Dirichlet via anti-bounce-back.** `antiBounceBackScalar(post, i, c, phiWall) =
  -post(opposite[i], c) + 2 t_i phiWall`, in `boundary/bounce_back.hpp`. Places the fixed value at
  the half-node wall, consistent with the existing halfway bounce-back convention.
- **Buoyancy = per-cell force (reuse everything).** `applyBuoyancy<Backend>(fluid, temperature, p)`
  in `timestep/buoyancy.hpp` mirrors `copyVelocityToExternal`: geometry check, null-dynamics skip,
  `forEachIndex` writing `F[gravityAxis] = rho·g·β·(phi − T_ref)` (rho from fluid dynamics or
  `rho0`), other components 0, into the fluid's external force block. The fluid uses
  `ForcedD2Q9` + `ExternalForceBGKdynamics`.
- **RayleighBenard2D step:** (1) `copyVelocityToExternal(fluid → temperature)`; (2)
  `applyBuoyancy(temperature → fluid)`; (3) collide+stream fluid (no-slip y-walls, periodic x);
  (4) collide+stream temperature (hot/cold anti-bounce-back y-walls, periodic x). Diagnostics:
  `avgKineticEnergy()`, `horizontallyAveragedT(y)`.
- **Dimensionless mapping for clean tests.** Fix `Pr`, `U_f`, `H=ny`; derive
  `nu = U_f·H/sqrt(Ra/Pr)`, `kappa = nu/Pr`, `g·β·dT = U_f²/H`. With these fixed, `g·β·dT` is
  **independent of Ra**, so sub- and super-critical runs share box/gravity/perturbation and differ
  ONLY in the two `omega` values — a controlled A/B onset test. Wall midpoint convention: `H=ny`,
  conduction profile `T(y) = T_hot + (T_cold − T_hot)·(y+0.5)/ny`.

## Risks / Trade-offs

- **VTK ordering bug (storage Z-fastest vs VTK X-fastest)** → the round-trip test uses an injective
  `s(x,y,z)=x+1000y+1e6z` so any transpose is caught.
- **Onset test flakiness near Ra_c** → use Ra well away from 1708 (≈800 vs ≈8000) and assert the
  KE trend (decay vs growth over a window), not a precise value.
- **Compressibility at U_f=0.1 (Ma≈0.17)** → acceptable for a qualitative onset test; documented.

## Open Questions

- Two-way coupling `rho`: local fluid density vs `rho0=1`? Default local; `rho0` option provided.
- Whether to fold this VTK writer into the solvers as a `writeVtk(path)` method now, or keep it
  free-standing and let examples wire it (leaning: a convenience method on the RB solver only).
