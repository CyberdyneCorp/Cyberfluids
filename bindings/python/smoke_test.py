#!/usr/bin/env python3
"""Python binding smoke test: build and run a cavity from Python, read the
velocity field as a NumPy array, and check the lid-driven flow is physical.
Exits non-zero on failure."""

import sys

import numpy as np

import cyberfluids as cf


def main() -> int:
    print("cyberfluids version:", cf.version())
    print("library:", cf.library_path())

    n = 32
    cav = cf.Cavity2D(n, n, omega=1.0, lid_velocity=0.1)
    assert cav.nx == n and cav.ny == n
    cav.run(3000)

    u = cav.velocity()
    rho = cav.density()
    assert u.shape == (n, n, 2), u.shape
    assert rho.shape == (n, n), rho.shape
    assert np.all(np.isfinite(u)) and np.all(np.isfinite(rho))

    speed = np.linalg.norm(u, axis=-1)
    assert speed.max() < 0.3, f"unstable: max speed {speed.max()}"
    assert 0.7 < rho.min() and rho.max() < 1.3, f"density out of range [{rho.min()},{rho.max()}]"

    # Lid drives flow: u_x larger near the top than the bottom (center column).
    ux_top = u[n // 2, n - 2, 0]
    ux_bot = u[n // 2, 1, 0]
    assert ux_top > 0.0 and ux_top > ux_bot, f"no lid-driven asymmetry: {ux_top} vs {ux_bot}"

    cav.close()
    print(f"python smoke OK (max speed {speed.max():.4f}, ux_top {ux_top:.4f})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
