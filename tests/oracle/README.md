# Oracle validation — Palabos reference

Cyberfluids is validated against [Palabos](https://gitlab.com/unigespc/palabos) as a
numerical oracle: the same lid-driven cavity is run in both codes and their steady-state
centerline velocity profiles are compared. The Cyberfluids regression test
(`tests/test_oracle_cavity.cpp`) reads the stored reference below and compares in-process —
it does **not** build or link Palabos.

## Reference

- `cavity2d_palabos.csv` — steady-state centerline velocities (lattice units) from Palabos.

## Matched parameters (must stay in sync across the three files)

`tests/oracle/palabos/cavity2d_oracle.cpp` and `tests/test_oracle_cavity.cpp`:

| Parameter | Value |
|---|---|
| Descriptor | D2Q9 |
| Collision | BGK |
| Grid | 64 × 64 |
| Lid velocity `U` | 0.05 (lattice units) |
| Relaxation | `omega = 1/0.6` (`tau = 0.6`) |
| Kinematic viscosity | `nu = cs2 (1/omega − 1/2) = 1/30` |
| Reynolds number | `Re = U·N/nu ≈ 96` |
| Iterations | 40 000 (steady state) |

## Comparison method and tolerance

The two codes use **different velocity boundary schemes** — Cyberfluids uses moving-wall
bounce-back on the lid; the Palabos program uses an interpolation (Skordos) velocity BC.
This is therefore a **physics-level** comparison, not a bit-level one:

- Compared quantities: `u_x`, `u_y` along the vertical centerline (x = N/2) and the
  horizontal centerline (y = N/2).
- **Interior nodes only** (boundary rows excluded), since the wall treatment differs by
  construction there.
- Deviations are normalized by the lid speed `U`.
- Tolerances (fraction of `U`): **L∞ ≤ 0.07**, **L2 (RMS) ≤ 0.015**.

Observed agreement: **L∞ ≈ 0.050, L2 ≈ 0.008**. The L∞ peak sits at the interior node
directly below the lid (vertical centerline, y = 62), where moving-wall bounce-back and the
Palabos interpolation BC differ most; the overall RMS agreement is ~0.8% of `U`. The test
prints the observed `Linf` (and its location) and `L2`, so a regression shows up as a rising
number before it breaches the tolerance.

## Palabos revision

- Repo: https://gitlab.com/unigespc/palabos
- Commit: `4127697e90169bbef982295f1d1c933cf6e90caa` (`4127697`, branch master)

## Regenerating the reference

```bash
cd tests/oracle/palabos
cmake -S . -B build -DPALABOS_ROOT=/path/to/palabos -DCMAKE_BUILD_TYPE=Release
cmake --build build -j          # builds the palabos static lib + generator
./build/cavity2d_oracle          # writes cavity2d_palabos.csv here
cp cavity2d_palabos.csv ../cavity2d_palabos.csv
```

Notes:
- Serial build (no MPI), no HDF5.
- libc++ gates the parallel STL behind `-fexperimental-library`; the CMake adds it
  automatically when the compiler supports it (Palabos uses `std::execution::par_unseq`).
- Requires TBB (`brew install tbb`).
