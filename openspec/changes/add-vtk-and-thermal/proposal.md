## Why

Two high-value gaps remain that are now cheap given the external-field work: there is no way to
**see** results (no field export), and there is no **thermal** model despite the AD dynamics,
per-cell forcing, and one-way coupling all being in place. This change adds VTK output (so any
run can be opened in ParaView) and a Boussinesq thermal model with two-way fluid↔temperature
coupling, demonstrated by Rayleigh–Bénard convection.

## What Changes

- Add a header-only **`VtkStructuredWriter<Dim>`** (`io/vtk_writer.hpp`) that writes legacy VTK
  `STRUCTURED_POINTS` (ASCII, no external deps) with named scalar/vector fields, in the correct
  X-fastest point ordering (2D → nz=1; vectors always 3 components).
- Add **`antiBounceBackScalar`** — a fixed-value (Dirichlet) scalar wall for AD lattices.
- Add **`applyBuoyancy<Backend>(fluid, temperature, params)`** (`timestep/buoyancy.hpp`): the
  two-way feedback writing `F = rho·g·β·(T − T_ref)` into the fluid's per-cell external force,
  mirroring `copyVelocityToExternal`.
- Add a **`RayleighBenard2D`** solver (ForcedD2Q9 fluid + AdvectedD2Q5 temperature; hot bottom /
  cold top Dirichlet walls; no-slip fluid walls; periodic sides) with a dimensionless
  `fromDimensionless(Ra, Pr, U_f, H, nx)` factory and diagnostics (kinetic energy, T profile).
- Validate: VTK round-trip parse; buoyancy force-map unit test; pure-conduction analytic profile
  (gravity off); sub- vs super-critical Rayleigh–Bénard (Ra_c ≈ 1708) — same box/gravity, differ
  only in `omega`.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `geometry-and-io`: the **Field output for visualization** requirement is made concrete (legacy
  VTK `STRUCTURED_POINTS`, the writer API, ordering).
- `physical-models`: the **Thermal flow (Boussinesq)** requirement gains the concrete mechanism
  (AD temperature lattice + per-cell buoyancy force + two-way coupling).
- `external-fields`: add a **two-way (buoyancy) coupling** requirement alongside the one-way one.
- `boundary-conditions`: add a **scalar Dirichlet (anti-bounce-back)** requirement.

## Impact

- New headers: `io/vtk_writer.hpp`, `timestep/buoyancy.hpp`, `solver/rayleigh_benard.hpp`; edit
  `boundary/bounce_back.hpp`. New example `rayleigh_benard`. New tests.
- No change to existing dynamics, the one-way coupling, the C ABI, or existing solvers.

## Non-Goals

- Binary VTK / XML `.vti` (ASCII only for now); parallel/partitioned VTK.
- 3D Rayleigh–Bénard; turbulent/high-Ra regimes; Nusselt-number precision (loose check only).
