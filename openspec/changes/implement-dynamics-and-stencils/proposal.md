## Why

The baseline specs require a family of collision models (BGK, TRT, MRT, regularized, forced)
and the D2Q9/D3Q19/**D3Q27** hydrodynamic plus **D2Q5/D3Q7** advection-diffusion descriptors.
The bootstrap MVP shipped only BGK on D2Q9/D3Q19. M1 implements the next tier so Cyberfluids
can run at higher Reynolds numbers (TRT/MRT stability), with better 3D isotropy (D3Q27), with
projected non-equilibrium (regularized), and under a body force (forced — e.g. channel flow).

## What Changes

- Add lattice descriptors **D3Q27** (3D hydrodynamic), **D2Q5**, **D3Q7** (AD stencils), each
  Concept-validated with the same physical-invariant tests as D2Q9/D3Q19.
- Add **TRT** (two-relaxation-time) collision dynamics with a magic-parameter convention.
- Add **MRT** (multiple-relaxation-time) collision dynamics for **D2Q9** (moment-space
  relaxation via the d'Humières matrix).
- Add **Regularized BGK** dynamics (non-equilibrium projected onto the stress basis).
- Add **Forced BGK** dynamics with a **uniform body force** (Guo forcing).
- Add a **Poiseuille channel** example + regression test validating forcing against the
  analytic parabolic profile.
- Refine two baseline requirements to match the delivered scope (see Modified Capabilities).

## Capabilities

### New Capabilities
- None. M1 implements existing `collision-dynamics` and `lattice-descriptors` requirements.

### Modified Capabilities
- `collision-dynamics`: refine **Forced dynamics** to a **uniform (constant) body force** via
  Guo forcing (per-cell external-field forcing is deferred until external-field infrastructure
  lands).
- `lattice-descriptors`: refine **Advection-diffusion descriptors** to provide the **base
  D2Q5/D3Q7 stencils**; the advection-velocity external field and `WithSource` variants are
  deferred (they need the external-field infrastructure).

## Impact

- New headers: `core/descriptors.hpp` (+D3Q27/D2Q5/D3Q7), `dynamics/trt.hpp`, `dynamics/mrt.hpp`,
  `dynamics/regularized.hpp`, `dynamics/forced.hpp`.
- New example `examples/poiseuille2d.cpp`; new tests for each model + the Poiseuille profile.
- No changes to the C ABI, bindings, or the existing BGK cavity.

## Non-Goals

- Advection-diffusion **dynamics/coupling** (only the D2Q5/D3Q7 stencils are added).
- Per-cell external-field forcing; `WithSource` AD variants.
- MRT for D3Q19/D3Q27 (D2Q9 MRT only in M1).
