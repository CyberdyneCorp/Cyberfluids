## 1. Descriptors

- [x] 1.1 Add `D3Q27` descriptor (velocities, weights, opposites, cNormSqr, cs2=1/3)
- [x] 1.2 Add `D2Q5` and `D3Q7` base advection-diffusion stencils
- [x] 1.3 Extend the descriptor test to cover D3Q27/D2Q5/D3Q7 invariants (weight sum, 1st/2nd
      moments, opposites, cNormSqr, Concept satisfaction)

## 2. TRT dynamics

- [x] 2.1 Implement `TRTdynamics<T,Descriptor>(omega_plus, magic=1/4)` with omega_minus from the magic parameter
- [x] 2.2 Reuse BGK equilibrium/moments; relax symmetric/antisymmetric parts
- [x] 2.3 Test: reduces to BGK when omega_minus==omega_plus; mass/momentum conserved; equilibrium fixed point

## 3. MRT dynamics (D2Q9)

- [x] 3.1 Encode the D2Q9 moment matrix M and its inverse as constexpr tables
- [x] 3.2 Implement `MRTdynamics<T,D2Q9>(omega)` — moment-space relaxation with conserved moments unrelaxed
- [x] 3.3 Test: reduces to BGK when all free rates == omega; conservation; equilibrium fixed point

## 4. Regularized dynamics

- [x] 4.1 Implement `RegularizedBGKdynamics<T,Descriptor>(omega)` (Hermite projection of f_neq)
- [x] 4.2 Test: conservation; equilibrium fixed point; regularized f_neq reproduces the input Pi_neq

## 5. Forced dynamics + Poiseuille

- [x] 5.1 Implement `ForcedBGKdynamics<T,Descriptor>(omega, force)` with Guo forcing + half-force velocity
- [x] 5.2 Build a periodic Poiseuille channel solver (bounce-back walls, uniform streamwise force)
- [x] 5.3 Test: steady-state profile matches the analytic parabola within tolerance; example `poiseuille2d`

## 6. Integration & validation

- [x] 6.1 Add all new dynamics to the umbrella header; wire new tests into CTest
- [x] 6.2 Confirm the new models run through the existing cavity solver (swap BGK→TRT/MRT/regularized) without core changes
- [x] 6.3 `openspec validate --all --strict` passes; full suite green
- [x] 6.4 Update docs/features.md status for collision-dynamics and lattice-descriptors
