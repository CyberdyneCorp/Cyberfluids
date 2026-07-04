"""Cyberfluids Python binding.

A thin ctypes wrapper over the Cyberfluids C ABI (libcyberfluids_c). Macroscopic
fields are returned as NumPy arrays that mirror the NumPP-backed C++ fields, so
results flow into the scientific-Python stack. See
openspec/specs/language-bindings/spec.md.

Locating the shared library (first that loads wins):
  1. $CYBERFLUIDS_LIBRARY (explicit path)
  2. the repo build tree (build/, build/lib/)
  3. the system loader (ctypes.util.find_library)
"""

from __future__ import annotations

import ctypes
import ctypes.util
import os
import pathlib

import numpy as np

__all__ = ["Cavity2D", "version", "library_path"]

_LIB_NAMES = ("libcyberfluids_c.dylib", "libcyberfluids_c.so", "cyberfluids_c.dll")


def _candidate_paths():
    env = os.environ.get("CYBERFLUIDS_LIBRARY")
    if env:
        yield env
    here = pathlib.Path(__file__).resolve()
    # bindings/python/cyberfluids/__init__.py -> repo root is parents[3]
    repo = here.parents[3]
    for d in (repo / "build", repo / "build" / "lib"):
        for name in _LIB_NAMES:
            yield str(d / name)
    found = ctypes.util.find_library("cyberfluids_c")
    if found:
        yield found


def _load_library():
    tried = []
    for path in _candidate_paths():
        if path and os.path.exists(path):
            return ctypes.CDLL(path), path
        tried.append(path)
    raise OSError(
        "could not locate libcyberfluids_c. Set $CYBERFLUIDS_LIBRARY to its path. "
        f"Tried: {tried}"
    )


_lib, _LIBRARY_PATH = _load_library()

# --- C ABI signatures ---
_lib.cf_version.restype = ctypes.c_char_p

_lib.cf_cavity2d_create.restype = ctypes.c_void_p
_lib.cf_cavity2d_create.argtypes = [ctypes.c_int64, ctypes.c_int64, ctypes.c_double, ctypes.c_double]
_lib.cf_cavity2d_destroy.argtypes = [ctypes.c_void_p]
_lib.cf_cavity2d_run.argtypes = [ctypes.c_void_p, ctypes.c_int64]
_lib.cf_cavity2d_step.argtypes = [ctypes.c_void_p]
_lib.cf_cavity2d_nx.restype = ctypes.c_int64
_lib.cf_cavity2d_nx.argtypes = [ctypes.c_void_p]
_lib.cf_cavity2d_ny.restype = ctypes.c_int64
_lib.cf_cavity2d_ny.argtypes = [ctypes.c_void_p]
_lib.cf_cavity2d_velocity.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_double)]
_lib.cf_cavity2d_density.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_double)]


def version() -> str:
    """Cyberfluids library version string."""
    return _lib.cf_version().decode()


def library_path() -> str:
    """Filesystem path of the loaded C ABI shared library."""
    return _LIBRARY_PATH


class Cavity2D:
    """A 2D lid-driven cavity (D2Q9, BGK) driven through the C++ core."""

    def __init__(self, nx: int, ny: int, omega: float, lid_velocity: float):
        self._h = _lib.cf_cavity2d_create(nx, ny, omega, lid_velocity)
        if not self._h:
            raise ValueError("cf_cavity2d_create failed (nx and ny must be > 0)")

    @property
    def nx(self) -> int:
        return _lib.cf_cavity2d_nx(self._h)

    @property
    def ny(self) -> int:
        return _lib.cf_cavity2d_ny(self._h)

    def run(self, steps: int) -> None:
        _lib.cf_cavity2d_run(self._h, steps)

    def step(self) -> None:
        _lib.cf_cavity2d_step(self._h)

    def velocity(self) -> np.ndarray:
        """Velocity field as a NumPy array of shape (nx, ny, 2)."""
        out = np.empty(self.nx * self.ny * 2, dtype=np.float64)
        _lib.cf_cavity2d_velocity(self._h, out.ctypes.data_as(ctypes.POINTER(ctypes.c_double)))
        return out.reshape(self.nx, self.ny, 2)

    def density(self) -> np.ndarray:
        """Density field as a NumPy array of shape (nx, ny)."""
        out = np.empty(self.nx * self.ny, dtype=np.float64)
        _lib.cf_cavity2d_density(self._h, out.ctypes.data_as(ctypes.POINTER(ctypes.c_double)))
        return out.reshape(self.nx, self.ny)

    def close(self) -> None:
        if getattr(self, "_h", None):
            _lib.cf_cavity2d_destroy(self._h)
            self._h = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
