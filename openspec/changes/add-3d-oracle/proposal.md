## Why

The 2D cavity is Palabos-validated, but the 3D cavity was only behaviorally tested — a real
confidence gap for the D3Q19 path. This extends the oracle to 3D: a matched Palabos D3Q19 cavity
reference and an in-process comparison, closing that gap.

## What Changes

- Add `tests/oracle/palabos/cavity3d_oracle.cpp` — a Palabos D3Q19 lid-driven cavity matching
  `LidDrivenCavity3D` (lid on the top z-face interior driving +x, no-slip walls, same N/omega/U/steps),
  dumping steady-ish centerline velocity profiles to CSV. Built by the existing Palabos harness.
- Store `tests/oracle/cavity3d_palabos.csv` as the reference.
- Add `test_oracle_cavity3d` — mirrors the 2D oracle: interior-node L2/L∞ of the centerline
  velocities normalized by U, documented tolerance, reports peak deviation + location.

## Capabilities

### Modified Capabilities
- `oracle-validation`: the **Palabos as numerical oracle** requirement now covers both the 2D and
  3D lid-driven cavities.

## Impact

- New `cavity3d_oracle.cpp` (+ one line in the Palabos harness CMake), the stored 3D CSV, and
  `test_oracle_cavity3d` (gated on the CSV, like the 2D oracle). No core changes.

## Non-Goals

- Larger/steadier 3D runs (kept at N=20 / 8000 steps for a fast, developed-flow comparison);
  matching Palabos's exact wall BC (physics-level comparison, as in 2D).

## Notes

- The Palabos 3D umbrella (`palabos3D.hh`) does not compile under AppleClang (a parallel
  `exclusive_scan` in the accelerated lattice and an `offLattice` bug); the generator includes a
  reduced set of Palabos module headers instead. Documented in `tests/oracle/README.md`.
