"""Smoke test for the WindTunnel Python binding (analytic sphere — no CMG needed).
Self-skips if numpy is absent. Runs a small case and checks the field shape,
stability, no-slip inside the solid, and a momentum-deficit wake."""

import sys

try:
    import numpy as np
except ImportError:
    print("SKIP wind_tunnel python test: numpy not installed")
    sys.exit(0)

import cyberfluids as cf

failures = 0


def check(cond, label):
    global failures
    if not cond:
        failures += 1
        print(f"FAIL: {label}")


omega = cf.WindTunnel.omega_for_reynolds(0.05, 8.0, 20.0)
with cf.WindTunnel(48, 24, 24, omega, 0.05) as t:
    t.set_sphere(14, 12, 12, 4.0)
    t.run(2000)

    v = t.velocity()
    check(v.shape == (48, 24, 24, 3), f"velocity shape {v.shape}")
    speed = np.linalg.norm(v, axis=-1)
    check(bool(np.isfinite(speed).all()), "velocity not finite (diverged)")
    check(float(speed.max()) < 0.5, f"speed too high {float(speed.max())}")

    solid = t.solid()
    check(solid.shape == (48, 24, 24), f"solid shape {solid.shape}")
    check(int((solid > 0.5).sum()) > 0, "no solid cells")

    check(float(speed[14, 12, 12]) < 0.1 * 0.05, "no-slip inside solid failed")
    check(float(v[22, 12, 12, 0]) < float(v[4, 6, 6, 0]), "no wake behind sphere")

if failures == 0:
    print("wind_tunnel python: all checks passed")
sys.exit(failures)
