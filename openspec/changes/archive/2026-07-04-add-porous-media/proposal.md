# Add porous media (partial bounce-back)

## Why

The `physical-models` baseline already promises partial bounce-back for porous
media, but only in the abstract ("resist flow proportionally to its solid
fraction"). This change delivers the concrete numerical operator, per-cell
solid-fraction storage, a driver, and a validated formulation so the capability
is real and testable.

## What Changes

- Add an `ext::Scalar<N>` external-field trait (a generic per-cell scalar block,
  distinct from the existing force/velocity blocks) plus a `numScalarScalars<D>()`
  detector, mirroring the force/velocity traits.
- Add `descriptors::PorousD2Q9` / `PorousD3Q19` = base stencil + `ext::Scalar<1>`
  holding the per-cell solid fraction `ns`.
- Add `PorousForcedBGKdynamics<T, Descriptor>`: BGK collision with a uniform Guo
  body force blended with node bounce-back by the Walsh, Burr & Holmes (2009)
  linear rule, in its convex form
  `f_i^out = (1-ns)·f_i^{forced-BGK,out} + ns·f_opposite(i)`. `ns=0` reduces to
  forced BGK exactly; `ns=1` becomes the exact population swap (no-slip node);
  the body force is attenuated by `(1-ns)` so it vanishes in solid cells.
- Add `solver::PorousBox2D`: a fully-periodic uniform-`ns` forced Darcy box that
  exposes the through-flow (flux) velocity and the analytic oracle.
- Add `tests/test_porous.cpp` covering the exact `ns=0`/`ns=1` identities and the
  analytic Darcy law.

## Non-goals

- Per-cell (spatially varying) body force in the porous operator — the force is a
  uniform dynamics member; only `ns` varies per cell. (A graded-`ns` setter is
  provided; graded force is out of scope.)
- A walled porous channel solver and a Palabos oracle — the closed-form Darcy
  flux is a stronger, self-contained oracle, so no external reference is needed.
