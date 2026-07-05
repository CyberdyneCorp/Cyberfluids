"""DLPack zero-copy interop test for the Python binding.

Self-skips (exit 0) when numpy is missing / too old / lacks from_dlpack, so it is
safe to register unconditionally in CI. When numpy>=1.23 is present it proves the
imported array shares memory with the C++ lattice buffer (no copy)."""

import sys


def _skip(msg):
    print(f"SKIP dlpack python test: {msg}")
    sys.exit(0)


try:
    import numpy as np
except ImportError:
    _skip("numpy not installed")

_ver = tuple(int(p) for p in np.__version__.split(".")[:2])
if _ver < (1, 23) or not hasattr(np, "from_dlpack"):
    _skip(f"numpy {np.__version__} lacks from_dlpack (need >= 1.23)")

import ctypes

import cyberfluids as cf

failures = 0


def check(cond, label):
    global failures
    if not cond:
        failures += 1
        print(f"FAIL: {label}")


def c_data_ptr(field):
    """Data pointer the C exporter would hand out for this field, without going
    through a consumer. Frees the probe tensor immediately (releases only its
    buffer *reference*, not the shared buffer)."""
    mt = field._produce(field._h)
    ptr = ctypes.cast(mt.contents.dl_tensor.data, ctypes.c_void_p).value
    mt.contents.deleter(mt)
    return ptr


with cf.Cavity2D(16, 16, 1.0 / 0.6, 0.05) as cav:
    cav.run(20)

    # (1) Density field: shape, dtype, physical sanity, and zero-copy aliasing.
    dfield = cav.density_field()
    probe = c_data_ptr(dfield)
    rho = np.from_dlpack(dfield)
    check(rho.shape == (16, 16), f"density shape {rho.shape} != (16,16)")
    check(rho.dtype == np.float64, f"density dtype {rho.dtype}")
    check(abs(float(rho.mean()) - 1.0) < 0.05, f"density mean {float(rho.mean())}")
    # Zero-copy proof: the numpy view's buffer is the very same C++ buffer.
    check(rho.__array_interface__["data"][0] == probe, "density view is not zero-copy")

    # (2) Velocity field shape + zero-copy.
    vfield = cav.velocity_field()
    vprobe = c_data_ptr(vfield)
    vel = np.from_dlpack(vfield)
    check(vel.shape == (16, 16, 2), f"velocity shape {vel.shape} != (16,16,2)")
    check(vel.__array_interface__["data"][0] == vprobe, "velocity view is not zero-copy")

    # (3) Populations: shape {9, nx*ny} + zero-copy (no step between, so the
    #     buffer is not swapped and the pointer is stable).
    pfield = cav.populations()
    pprobe = c_data_ptr(pfield)
    pop = np.from_dlpack(pfield)
    check(pop.shape == (9, 16 * 16), f"populations shape {pop.shape}")
    check(pop.__array_interface__["data"][0] == pprobe, "populations view is not zero-copy")

    # (4) Optional torch interop, if available.
    try:
        import torch

        t = torch.from_dlpack(cav.populations())
        check(tuple(t.shape) == (9, 16 * 16), f"torch shape {tuple(t.shape)}")
    except ImportError:
        pass

if failures == 0:
    print("dlpack python: all checks passed")
sys.exit(failures)
