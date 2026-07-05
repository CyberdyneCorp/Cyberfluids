# Tasks

## 1. Storage
- [x] 1.1 Add `ext::Scalar<N>` trait to `core/external_traits.hpp`
- [x] 1.2 Add `numScalarScalars<D>()` detector
- [x] 1.3 Add `descriptors::PorousD2Q9` / `PorousD3Q19` + static_asserts

## 2. Dynamics
- [x] 2.1 Add `PorousForcedBGKdynamics<T, Descriptor>` (`dynamics/porous.hpp`),
      convex Walsh blend with attenuated Guo force, snapshotting pre-collision
      populations for the bounce-back term
- [x] 2.2 `static_assert(numScalarScalars<Descriptor>() >= 1)`

## 3. Solver
- [x] 3.1 Add `solver::PorousBox2D` (periodic Darcy box): uniform/graded `ns`
      setters, `meanFluxVelocityX()`, `analyticFluxVelocityX(ns)`

## 4. Validation
- [x] 4.1 `tests/test_porous.cpp`: T1 `ns=0`==BGK exact, T2 `ns=1`==swap exact
      (+ mass/momentum), T3 monotonic flow vs `ns`, T4 analytic Darcy + linearity
- [x] 4.2 Register `porous` in `tests/CMakeLists.txt`; all tests pass

## 5. Spec & docs
- [x] 5.1 Modify the `physical-models` "Porous media" requirement with the
      concrete formula, exact limits, and Darcy scenario
- [x] 5.2 `openspec validate --all --strict`
- [x] 5.3 Adversarial review + fix findings (4 confirmed test-rigor findings:
      tightened T4 to the exact 1e-9 oracle, dropped 2 vacuous assertions, added
      T5 spatial-ns mass-conservation exercising streaming)
- [x] 5.4 Archive
