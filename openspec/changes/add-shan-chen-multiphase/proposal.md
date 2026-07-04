## Why

The physical-models spec calls for Shan-Chen pseudopotential multiphase, but it isn't
implemented. The external-field infrastructure (per-cell force + Guo forcing) now makes the
single-component model cheap: the interaction force is just a non-local pre-collide pass writing
into the fluid's external force, exactly like buoyancy. This adds spontaneous liquid/vapour
phase separation and surface tension — the "oil and water" headline in single-component form.

## What Changes

- Add `physics/shan_chen_eos.hpp`: the pseudopotential `psi(rho) = rho0 (1 - e^{-rho/rho0})` and
  the equation-of-state pressure `p = cs2 rho + (cs2 G / 2) psi^2`.
- Add `timestep/shan_chen.hpp`: `computeDensityField` and a non-local, periodic-wrap
  `applyShanChenForce<Backend>` writing `F(x) = -G psi(rho(x)) sum_i w_i psi(rho(x+c_i)) c_i`
  (SC weights `w_i = invCs2 * t_i`) into the fluid's per-cell external force.
- Add `solver/shan_chen_2d.hpp`: `ShanChen2D` (ForcedD2Q9 + ExternalForceBGKdynamics, fully
  periodic; step = density field → interaction force → collide → stream).
- Validate: exact force (incl. periodic wrap); homogeneity → zero force; spontaneous separation
  above the critical `G` (and homogeneity below it); mass conservation; Laplace law (ΔP ∝ 1/R).

## Capabilities

### Modified Capabilities
- `physical-models`: the **Multi-component / multi-phase fluids** requirement gains a concrete
  single-component pseudopotential scenario (spontaneous separation + surface tension).

## Impact

- New headers: `physics/shan_chen_eos.hpp`, `timestep/shan_chen.hpp`, `solver/shan_chen_2d.hpp`;
  new tests; a `shan_chen` example. Reuses `ForcedD2Q9` + `ExternalForceBGKdynamics` unchanged;
  no core changes.

## Non-Goals

- Multi-component Shan-Chen (two coupled species); 3D; other pseudopotentials; thermal coupling.
