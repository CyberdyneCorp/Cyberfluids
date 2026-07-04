## 1. Generator + reference

- [x] 1.1 Add `tests/oracle/palabos/cavity3d_oracle.cpp` (Palabos D3Q19, matched to LidDrivenCavity3D; reduced module includes to dodge the AppleClang-incompatible accelerated/offLattice headers)
- [x] 1.2 Add `cavity3d_oracle` to the Palabos harness CMake; build + run; store `tests/oracle/cavity3d_palabos.csv`

## 2. Comparison test

- [x] 2.1 Add `test_oracle_cavity3d` (mirrors the 2D oracle: interior-node L2/L∞ vs U; reports peak location); CMake gated on the 3D CSV
- [x] 2.2 Calibrate tolerance (observed L∞≈0.13 near-lid, L2≈0.018 bulk at N=20); suite green

## 3. Docs

- [x] 3.1 Add a 3D section to `tests/oracle/README.md` (params, tolerance, reduced-include note); update feature table (pending)
