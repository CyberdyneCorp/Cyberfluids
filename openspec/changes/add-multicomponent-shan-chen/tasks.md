## 1. Inter-species force

- [x] 1.1 Add `applyInterComponentForce<Backend>(fluid, rhoSelf, rhoOther, G)` to `timestep/shan_chen.hpp` (linear psi=rho, w_i=t_i, periodic wrap, writes external force)

## 2. Solver

- [x] 2.1 Add `solver/multi_component_shan_chen_2d.hpp`: `MultiComponentShanChen2D` (two ForcedD2Q9 lattices; step = both densities → both cross-forces → collide+stream both); `initMixed`, `densityA/B`, `totalMass`, `segregation`

## 3. Validation

- [x] 3.1 Test `test_multi_component_shan_chen`: exact cross-force incl. periodic wrap; uniform-other → zero; de-mixing (G super) vs homogeneous (G sub); per-species mass conserved
- [x] 3.2 CTest wiring; suite green
- [ ] 3.3 Adversarial review + docs update (pending)
