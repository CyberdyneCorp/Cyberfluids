## Why

Single-component Shan-Chen (liquid/vapour) is in; the true "oil + water" story needs two
immiscible species. The per-cell external-force plumbing makes this a small, additive extension:
two fluid lattices coupled by a repulsive inter-species pseudopotential force.

## What Changes

- Add `applyInterComponentForce<Backend>(fluid, rhoSelf, rhoOther, G)` (in `timestep/shan_chen.hpp`):
  the multicomponent force `F_sigma(x) = -G psi(rho_self(x)) sum_i w_i psi(rho_other(x+c_i)) c_i`
  with linear `psi = rho`, weights `w_i = t_i`, periodic wrap, written into the species' external force.
- Add `solver::MultiComponentShanChen2D` — two `ForcedD2Q9` + `ExternalForceBGKdynamics` lattices
  stepped in lockstep (both densities → both cross-forces → collide+stream both), with `initMixed`,
  per-species density/mass, and a `segregation` order parameter.
- Validate: exact cross-force (incl. periodic wrap); uniform-other → zero force; de-mixing above the
  critical `G` (segregation grows) vs homogeneous below it; per-species mass conserved.

## Capabilities

### Modified Capabilities
- `physical-models`: the **Multi-component / multi-phase fluids** requirement gains the concrete
  two-species immiscible coupling (was "future extension").

## Impact

- Edits `timestep/shan_chen.hpp` (adds the free function); new `solver/multi_component_shan_chen_2d.hpp`;
  new test. Reuses `computeDensityField`, `ExternalForceBGKdynamics`, `ForcedD2Q9` unchanged.

## Non-Goals

- The rigorous shared-barycentric-velocity coupling (only correct for unequal viscosities); >2
  species; 3D; a saturating pseudopotential (the linear `psi=rho` has a narrow stable `G` window).
