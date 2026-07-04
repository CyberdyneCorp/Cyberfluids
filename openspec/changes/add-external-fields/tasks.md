## 1. External-field trait + storage core

- [x] 1.1 Add `core/external.hpp`: `ext::Force<D>` / `ext::Velocity<D>` traits, `WithExternal<Base,Ext>` wrapper, `numExternalScalars<D>()` (requires-expression, 0 when absent)
- [x] 1.2 Add `ExternalField<T,Descriptor>` (numpp `{nExt,ncells}`; guarded empty when nExt==0; `origin(c)` nullptr without `typed_data`; `operator()(s,c)`, `ncells()`, `array()`)
- [x] 1.3 Add variant typedefs in `descriptors.hpp` (`ForcedD2Q9/D3Q19/D3Q27`, `AdvectedD2Q5/D3Q7`) + `static_assert(LatticeDescriptor<...>)`; base descriptors unchanged

## 2. Cell + lattice wiring

- [x] 2.1 Extend `Cell`: defaulted `T* external=nullptr` ctor param + member; `external(int)`/const + `hasExternal()`; `operator[]`/`dynamics()` unchanged
- [x] 2.2 Add `ExternalField ext_` to `BlockLattice` (both ctors), `externalField()` accessors, and `ext_.origin(c)` in `cellAt`
- [x] 2.3 Test `test_external`: D2Q9 lattice has nExt==0 + null origin (no allocation); `ForcedD2Q9` round-trips external writes; existing lattice/bgk/cavity tests still pass

## 3. Per-cell forced dynamics

- [x] 3.1 Add `ExternalForceBGKdynamics<T,Descriptor>` to `forced.hpp` (omega-only ctor; reads force from `cell.external`; Guo math; static_assert nExt>=d); uniform class untouched
- [x] 3.2 Test `test_external_force`: uniform external force reproduces the Poiseuille parabola; a spatially varying force sanity check

## 4. Advection-diffusion dynamics

- [x] 4.1 Add `dynamics/advection_diffusion.hpp`: `AdvectionDiffusionBGKdynamics` (first-order equilibrium; phi=sum f; u from external; `isAdvectionDiffusion()`; static_assert nExt>=d)
- [x] 4.2 Test `test_advection_diffusion`: pure-diffusion decay vs analytic; constant-velocity advection on D2Q5/D3Q7

## 5. Fluid to AD coupling

- [x] 5.1 Add `timestep/coupling.hpp`: `copyVelocityToExternal<Backend>(fluid, ad)` (assert matching extents; forEachIndex writes fluid velocity into AD external slots)
- [x] 5.2 Test `test_ad_coupling`: velocity lands in AD external slots; mismatch assert fires; small end-to-end fluid→couple→AD scalar transport

## 6. Spec/doc update & validation

- [x] 6.1 Add new headers to the umbrella; wire the four new tests into CTest
- [x] 6.2 Update `docs/features.md` (physical-models/geometry note as applicable) and the feature table
- [x] 6.3 `openspec validate --all --strict` passes; full suite green
