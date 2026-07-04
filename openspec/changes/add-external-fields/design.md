## Context

Populations live in one SoA `numpp::ndarray {q, ncells}`; a `Cell` is a value view
(`origin=&f(0,c)`, `stride=ncells`, non-owning `Dynamics*`). This change adds node-local
per-cell external scalars without disturbing that hot layout. Design chosen via a 3-way design
workflow (minimal / Palabos-faithful / SoA-performance) + synthesis.

## Goals / Non-Goals

**Goals:** external-field storage that mirrors the SoA layout; descriptor opt-in that leaves the
base stencils and the concept untouched; per-cell forcing and AD dynamics; one-way fluid→AD
coupling. Keep the uniform forced dynamics, the BGK cavity, and the Poiseuille result working.

**Non-Goals:** two-way coupling; DRY-sharing Guo code with the uniform class; second-order AD
equilibrium; `WithSource` AD variants.

## Decisions

- **Separate parallel SoA tensor.** `ExternalField<T,Descriptor>` is a `numpp::ndarray
  {nExt, ncells}`, component-major, sharing the `ncells` stride. It is a *separate* allocation
  from the populations and is **never swapped by `streamPeriodic`** — external data is
  node-local (a body force / advection velocity belongs to a location, not a moving population).
- **Zero-cost absence.** `nExt = numExternalScalars<Descriptor>()`. When `nExt == 0` the field
  allocates nothing and `origin(c)` returns `nullptr` (never calls `typed_data` on an empty
  array). A plain-descriptor lattice pays nothing.
- **Compositional opt-in, base stencils untouched.** Traits `ext::Force<D>` / `ext::Velocity<D>`
  (declaring `numScalars` and the force/velocity offsets) plus one wrapper
  `WithExternal<Base, Ext> : Base { using ExternalField = Ext; }`. Detection is a free function
  `numExternalScalars<D>()` using `if constexpr (requires { D::ExternalField::numScalars; })` —
  so the `LatticeDescriptor` concept is unchanged and D2Q9/D3Q19/D3Q27/D2Q5/D3Q7 stay
  byte-for-byte identical. Variants satisfy the concept by inheriting the base's `constexpr`
  members.
- **Additive Cell change.** New defaulted 4th ctor param `T* external = nullptr`, member
  `external_`, and `external(int offset)` (+ const overload, since `computeVelocity` is const).
  `operator[]`/`dynamics()` unchanged, so existing Cell users are unaffected.
- **Per-cell forcing is a new sibling class.** `ExternalForceBGKdynamics` reads the force from
  `cell.external(forceBeginsAt + a)` and applies the same Guo math; the proven uniform
  `ForcedBGKdynamics` is left intact (no shared-helper refactor for the MVP).
- **AD dynamics use the first-order equilibrium** `feq_i = t_i phi (1 + invCs2 (c_i·u))` (NOT the
  second-order `BGKdynamics::equilibrium`); `phi = sum f_i`; `u` from the external field.
- **Coupling mirrors the evolve seam.** `copyVelocityToExternal<Backend>(fluid, ad)` asserts
  matching extents, then `Backend::forEachIndex` writes fluid velocity into AD external slots.
- **Compile-time misuse guard.** Each new dynamics `static_assert`s
  `numExternalScalars<Descriptor>() >= Descriptor::d`, turning a wrong descriptor into a clear
  compile error rather than a null external pointer dereference.

## Risks / Trade-offs

- **Empty-ndarray null deref** → `origin()`/allocation guarded by `if constexpr (nExt > 0)`;
  test asserts `nExt==0` ⇒ null origin, no allocation.
- **Concept regression** → keep base descriptors unchanged; a compile-only check confirms all
  five bases + the new variants satisfy `LatticeDescriptor`.
- **Forced regression** → per-cell forcing re-runs the exact Poiseuille test with a uniform
  external force and must reproduce the parabola.

## Open Questions

- Two-way coupling (scalar→force feedback, e.g. Boussinesq) — shape now or defer? (Deferred.)
- DRY the Guo code between uniform and per-cell forced classes as a follow-up? (Deferred.)
- A `setExternalForce(box, F)` convenience setter vs raw `externalField()` access for the MVP.
