# Add D3Q19 MRT collision

## Why

MRT (multiple-relaxation-time) collision is implemented only for D2Q9
(`MRTdynamics`, hard-gated to `numPop==9 && d==2`). MRT's per-moment relaxation
is far more stable than BGK at high Reynolds number, so 3D simulations need a
D3Q19 MRT operator to be usable at low viscosity.

## What Changes

- Add `MRTdynamics3D<T, Descriptor>` (D3Q19) using the standard d'Humières (2002)
  orthogonal 19-moment basis: map to moment space with M, relax each moment
  toward `M·feq` at its own rate, map back with `M^-1 = M^T diag(1/norm)`. Every
  free rate set to `omega` recovers BGK exactly, by construction.
- Add `ForcedMRTdynamics3D<T, Descriptor>` with moment-space Guo forcing
  `m* = m − s(m − meq) + (1 − s/2)·M·S`, reducing to `ForcedBGKdynamics` exactly.
- Add `solver::PoiseuilleChannel3D<Dyn>` (D3Q19, periodic x/z, bounce-back y),
  templated on the dynamics so one test validates both MRT and BGK channels.
- Add `tests/test_mrt3d.cpp`.

## Non-goals

- D3Q27 MRT and MRT for the advection-diffusion stencils.
- Tuning the free (non-hydrodynamic) relaxation rates beyond the standard
  d'Humières stability defaults.
