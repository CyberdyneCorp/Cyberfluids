## Why

The MVP and M1 deliberately deferred per-cell external state: forced dynamics only supports a
uniform body force, and the D2Q5/D3Q7 advection-diffusion (AD) descriptors ship as bare
stencils with no way to carry a per-cell advection velocity. Both gaps have the same root
cause — there is no way for a cell to hold extra, node-local scalars. This change adds that
**external-field infrastructure** and uses it to unblock (1) spatially varying body forcing and
(2) AD dynamics coupled to a fluid velocity field (scalar transport).

## What Changes

- Add an **`ExternalField<T,Descriptor>`** — a separate Structure-of-Arrays `numpp` tensor of
  shape `{nExt, ncells}` sharing the populations' per-cell stride. It is **node-local**:
  streaming never moves it. A descriptor with no external fields allocates nothing (zero cost).
- Add **descriptor opt-in** via composable traits `ext::Force<D>` / `ext::Velocity<D>` and a
  single `WithExternal<Base, Ext>` wrapper; the base descriptors (D2Q9/D3Q19/D3Q27/D2Q5/D3Q7)
  stay byte-for-byte unchanged. Detection uses a C++20 `requires`-expression
  (`numExternalScalars<D>()`), so the `LatticeDescriptor` concept is unchanged.
- Extend **`Cell`** additively with a defaulted external pointer and `external(offset)` accessor
  (existing `operator[]`/`dynamics()` untouched).
- Add **`ExternalForceBGKdynamics`** (per-cell Guo forcing; the uniform `ForcedBGKdynamics` is
  left intact) and **`AdvectionDiffusionBGKdynamics`** (first-order AD equilibrium; reads the
  advection velocity from the external field).
- Add **`copyVelocityToExternal<Backend>(fluid, ad)`** — a backend-dispatched one-way coupling
  that writes a fluid lattice's velocity into an AD lattice's external velocity slots.
- Validate: external-field round-trip + zero-cost absence; per-cell forcing reproduces the
  Poiseuille parabola; AD pure-diffusion decay + constant-velocity advection; coupling transport.

## Capabilities

### New Capabilities
- `external-fields`: per-cell external-field storage, descriptor opt-in, Cell access, and the
  fluid→AD coupling seam.

### Modified Capabilities
- `collision-dynamics`: add **per-cell forced dynamics** and **advection-diffusion dynamics**
  requirements (the uniform forced requirement is unchanged).
- `lattice-descriptors`: the **Advection-diffusion descriptors** requirement is updated — the
  advection-velocity external-field variants (`AdvectedD2Q5`/`AdvectedD3Q7`) are now provided,
  closing the deferred clause.

## Impact

- New headers: `core/external.hpp`, `dynamics/advection_diffusion.hpp`, `timestep/coupling.hpp`;
  edits to `core/cell.hpp`, `core/lattice.hpp`, `core/descriptors.hpp`, `dynamics/forced.hpp`.
- No change to `PopulationField`, `streamPeriodic`, the C ABI, or the existing BGK cavity.

## Non-Goals

- Two-way coupling (e.g. Boussinesq scalar→force feedback) — one-way fluid→AD only.
- A DRY refactor sharing Guo code between the uniform and per-cell forced classes (deferred to
  avoid touching the proven Poiseuille path).
- Second-order AD equilibrium; `WithSource` (reaction) AD variants.
