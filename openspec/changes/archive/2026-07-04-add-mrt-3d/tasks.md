# Tasks

## 1. Dynamics
- [x] 1.1 `MRTdynamics3D<T, Descriptor>` (`dynamics/mrt3d.hpp`): 19x19 d'Humières
      M, row norms, relaxation vector; BGK-reduction by construction
- [x] 1.2 `ForcedMRTdynamics3D<T, Descriptor>`: moment-space Guo forcing

## 2. Solver
- [x] 2.1 `solver::PoiseuilleChannel3D<Dyn>` (periodic x/z, bounce-back y-walls)

## 3. Validation
- [x] 3.1 `tests/test_mrt3d.cpp`: T1 plain-MRT==BGK exact, T2 forced-MRT==forced-BGK
      exact, T3 3D channel recovers analytic parabola (<1%) and matches BGK
- [x] 3.2 Register `mrt3d` in `tests/CMakeLists.txt`; passes

## 4. Spec & review
- [x] 4.1 Modify the collision-dynamics "TRT and MRT models" requirement to cover D3Q19
- [x] 4.2 `openspec validate --all --strict`
- [x] 4.3 Adversarial review (clean — no confirmed findings)
- [x] 4.4 Archive
