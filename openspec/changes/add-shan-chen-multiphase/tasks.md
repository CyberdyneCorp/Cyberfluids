## 1. Pseudopotential EOS

- [x] 1.1 Add `physics/shan_chen_eos.hpp`: `shanChenPsi(rho, rho0)` and `shanChenPressure(rho, G, rho0, cs2)`

## 2. Interaction force

- [x] 2.1 Add `timestep/shan_chen.hpp`: `ShanChenParameters<T>{G, rho0}`, `computeDensityField<Backend>(fluid, rho*)`, `applyShanChenForce<Backend>(fluid, rho*, params)` (non-local, periodic wrap, `w_i = invCs2*t_i`, writes external force)
- [x] 2.2 Test `test_shan_chen`: exact force incl. periodic-wrap neighbour (analytic); uniform density → zero force

## 3. Solver + phase separation

- [x] 3.1 Add `solver/shan_chen_2d.hpp`: `ShanChen2D<Backend,T>` (ForcedD2Q9 + ExternalForceBGKdynamics, periodic; step = density → force → collide → streamPeriodic); `initDensity(mean, noiseAmp)`, `density`/`velocity`, `minMaxDensity`, `totalMass`
- [x] 3.2 Test: super-critical G=-6 spontaneously separates (large density gap); sub-critical G=-2 stays homogeneous; mass conserved

## 4. Laplace law + integration

- [x] 4.1 Test `test_shan_chen_laplace`: a droplet at a few radii — ΔP (from the EOS pressure) is linear in 1/R (positive slope = surface tension)
- [x] 4.2 Umbrella + CTest wiring; a `shan_chen` example that writes a VTK density snapshot
- [x] 4.3 `openspec validate --all --strict` + full suite green
