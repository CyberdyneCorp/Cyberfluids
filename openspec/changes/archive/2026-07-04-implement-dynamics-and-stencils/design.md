## Context

The MVP established the `Dynamics<T,Descriptor>` interface, `BGKdynamics`, the SoA
`PopulationField`, and the collide/stream loop. M1 adds four more collision models and three
descriptors against that interface — no core refactor. All new dynamics implement the same
virtual interface, so they drop into the existing `attributeDynamics` / `collide` machinery and
the cavity/Poiseuille solvers.

## Goals / Non-Goals

**Goals:**
- D3Q27, D2Q5, D3Q7 descriptors (Concept-valid, invariant-tested).
- TRT, MRT (D2Q9), Regularized, Forced (uniform Guo) collision models.
- Reuse the existing test harness; add analytic Poiseuille validation for forcing.

**Non-Goals:**
- Advection-diffusion dynamics/coupling; per-cell external-field forcing; `WithSource` AD.
- MRT for D3Q19/D3Q27.

## Decisions

- **TRT** — split populations into symmetric/antisymmetric parts
  `f_i^± = (f_i ± f_opp)/2`, relax at `omega_plus`/`omega_minus`:
  `f_i -= omega_plus (f_i^+ - feq_i^+) + omega_minus (f_i^- - feq_i^-)`.
  Constructor takes `omega_plus` (sets viscosity, same as BGK) and a **magic parameter**
  `Lambda` (default 1/4 for stability): `omega_minus = 1 / (Lambda/(1/omega_plus - 1/2) + 1/2)`.
  With `omega_minus = omega_plus` TRT reduces exactly to BGK — a direct test.
- **MRT (D2Q9)** — the d'Humières moment set `(rho, e, eps, jx, qx, jy, qy, pxx, pxy)` via the
  standard orthogonal matrix `M`; collide in moment space `m -= S (m - m_eq)` then map back with
  `M^{-1}`. Conserved moments (rho, jx, jy) have zero relaxation; the shear moments `pxx, pxy`
  relax at `s_nu = omega` (fixing viscosity); the remaining rates default to standard values but
  are set equal to `omega` in a test to verify **MRT reduces to BGK**. `M` and `M^{-1}` are
  `constexpr` tables specific to D2Q9 (hence D2Q9-only in M1).
- **Regularized BGK** — from `rho, u` build `feq`; take `f_neq = f - feq`; compute its second
  moment `Pi_neq`; rebuild a regularized `f_neq^reg` from `Pi_neq` via the Hermite projection;
  set `f = feq + (1 - omega) f_neq^reg`. Discards ghost modes → more stable.
- **Forced BGK (uniform Guo)** — constant force `F`. `u = (sum_i f_i c_i + F/2)/rho`; add
  `S_i = (1 - omega/2) w_i [ (c_i - u)/cs2 + (c_i.u)/cs2^2 c_i ] . F` after the BGK relaxation.
  A uniform force needs no per-cell storage, so this reuses the existing `Cell` unchanged.
- **Validation** — unit tests per model (equilibrium fixed point, mass/momentum conservation,
  BGK-equivalence limits for TRT and MRT). Forcing is validated against the **analytic
  Poiseuille parabola** `u(y) = F/(2 rho nu) y (L-y)` in a periodic channel with bounce-back
  walls — a closed-form oracle needing no external code.

## Risks / Trade-offs

- **MRT matrix transcription errors** → Mitigate with the BGK-equivalence test (all free rates =
  omega must reproduce BGK bit-for-bit up to rounding) and conservation checks.
- **TRT magic-parameter convention confusion** → Document the `Lambda` definition; test the
  `omega_minus = omega_plus` ⇒ BGK identity.
- **Poiseuille discretization error at coarse resolution** → use a channel wide enough that the
  parabola tolerance (a few % of u_max) is comfortably met; document it.

## Open Questions

- Default MRT relaxation rates for non-hydrodynamic moments (stability tuning) — start from the
  common literature values; revisit if a future MRT oracle comparison warrants.
