"""Cyberfluids Python binding.

A thin ctypes wrapper over the Cyberfluids C ABI (libcyberfluids_c). Macroscopic
fields are returned as NumPy arrays that mirror the NumPP-backed C++ fields, so
results flow into the scientific-Python stack. See
openspec/specs/language-bindings/spec.md.

Locating the shared library (first that loads wins):
  1. $CYBERFLUIDS_LIBRARY (explicit path)
  2. the library bundled in this package (installed wheel)
  3. the repo build tree (build/, build/lib/) for in-repo development
  4. the system loader (ctypes.util.find_library)
"""

from __future__ import annotations

import ctypes
import ctypes.util
import os
import pathlib

import numpy as np

__all__ = ["Cavity2D", "WindTunnel", "has_geometry", "version", "library_path"]

_LIB_NAMES = ("libcyberfluids_c.dylib", "libcyberfluids_c.so", "cyberfluids_c.dll")


def _candidate_paths():
    env = os.environ.get("CYBERFLUIDS_LIBRARY")
    if env:
        yield env
    here = pathlib.Path(__file__).resolve()
    # 1) bundled in this package (installed wheel: the .so sits next to __init__.py).
    for name in _LIB_NAMES:
        yield str(here.parent / name)
    # 2) the repo build tree, for `PYTHONPATH=bindings/python` in-repo development.
    #    bindings/python/cyberfluids/__init__.py -> repo root is parents[3].
    if len(here.parents) > 3:
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


# --- DLPack zero-copy interop -------------------------------------------------
# ctypes mirror of the vendored DLPack ABI (include/cyberfluids/interop/dlpack.h).
class _DLDevice(ctypes.Structure):
    _fields_ = [("device_type", ctypes.c_int), ("device_id", ctypes.c_int32)]


class _DLDataType(ctypes.Structure):
    _fields_ = [("code", ctypes.c_uint8), ("bits", ctypes.c_uint8), ("lanes", ctypes.c_uint16)]


class _DLTensor(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.c_void_p),
        ("device", _DLDevice),
        ("ndim", ctypes.c_int32),
        ("dtype", _DLDataType),
        ("shape", ctypes.POINTER(ctypes.c_int64)),
        ("strides", ctypes.POINTER(ctypes.c_int64)),
        ("byte_offset", ctypes.c_uint64),
    ]


class _DLManagedTensor(ctypes.Structure):
    pass


_DLDeleter = ctypes.CFUNCTYPE(None, ctypes.POINTER(_DLManagedTensor))
_DLManagedTensor._fields_ = [
    ("dl_tensor", _DLTensor),
    ("manager_ctx", ctypes.c_void_p),
    ("deleter", _DLDeleter),
]

for _fn in ("cf_cavity2d_populations_dlpack", "cf_cavity2d_velocity_dlpack",
            "cf_cavity2d_density_dlpack"):
    getattr(_lib, _fn).restype = ctypes.POINTER(_DLManagedTensor)
    getattr(_lib, _fn).argtypes = [ctypes.c_void_p]

# PyCapsule plumbing via the CPython C-API. A producer capsule is named
# "dltensor"; the consumer (numpy/torch from_dlpack) renames it to
# "used_dltensor" and calls the tensor's deleter when its view is released. Our
# capsule destructor only frees the tensor if the consumer never took ownership
# (i.e. the capsule is still named "dltensor"), avoiding a double free.
#
# The capsule is passed to and from the C-API as a raw pointer (c_void_p), NEVER
# as a ctypes py_object: a CFUNCTYPE callback with a py_object parameter makes
# ctypes steal and drop a reference to the capsule when the callback returns,
# corrupting its refcount and crashing the interpreter.
_PyCapsule_Destructor = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
_capsule_api = ctypes.pythonapi
_capsule_api.PyCapsule_New.restype = ctypes.py_object
_capsule_api.PyCapsule_New.argtypes = [ctypes.c_void_p, ctypes.c_char_p, _PyCapsule_Destructor]
_capsule_api.PyCapsule_IsValid.restype = ctypes.c_int
_capsule_api.PyCapsule_IsValid.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
_capsule_api.PyCapsule_GetPointer.restype = ctypes.c_void_p
_capsule_api.PyCapsule_GetPointer.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def _capsule_destructor(capsule_ptr):
    # capsule_ptr is a raw PyObject* (as an int); no refcount is touched.
    if _capsule_api.PyCapsule_IsValid(capsule_ptr, b"dltensor"):
        ptr = _capsule_api.PyCapsule_GetPointer(capsule_ptr, b"dltensor")
        mt = ctypes.cast(ptr, ctypes.POINTER(_DLManagedTensor))
        if mt and mt.contents.deleter:
            mt.contents.deleter(mt)


# Module-global so the CFUNCTYPE trampoline is not garbage-collected.
_CAPSULE_DTOR = _PyCapsule_Destructor(_capsule_destructor)


class _DLPackField:
    """A zero-copy DLPack view of a live lattice field. Pass to
    ``numpy.from_dlpack`` / ``torch.from_dlpack`` to build an array sharing the
    C++ buffer's memory (no copy)."""

    def __init__(self, produce, handle):
        self._produce = produce  # C function returning POINTER(_DLManagedTensor)
        self._h = handle

    def __dlpack_device__(self):
        return (1, 0)  # (kDLCPU, device 0)

    def __dlpack__(self, *, stream=None, max_version=None, dl_device=None, copy=None):
        mt = self._produce(self._h)
        return _capsule_api.PyCapsule_New(
            ctypes.cast(mt, ctypes.c_void_p), b"dltensor", _CAPSULE_DTOR
        )


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

    def populations(self) -> _DLPackField:
        """Zero-copy DLPack view of the populations, shape (9, nx*ny). Pass to
        numpy/torch.from_dlpack. NOTE: the populations buffer is swapped each
        step(), so build the view after the final step (or re-fetch each time)."""
        return _DLPackField(_lib.cf_cavity2d_populations_dlpack, self._h)

    def velocity_field(self) -> _DLPackField:
        """Zero-copy DLPack view of the velocity field, shape (nx, ny, 2).
        Refreshes the in-place macroscopic cache on consumption; stable across
        steps (re-fetch to see values after further stepping)."""
        return _DLPackField(_lib.cf_cavity2d_velocity_dlpack, self._h)

    def density_field(self) -> _DLPackField:
        """Zero-copy DLPack view of the density field, shape (nx, ny)."""
        return _DLPackField(_lib.cf_cavity2d_density_dlpack, self._h)

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


# --- 3D wind tunnel -----------------------------------------------------------
_P = ctypes.POINTER(ctypes.c_double)
_lib.cf_wind_tunnel_create.restype = ctypes.c_void_p
_lib.cf_wind_tunnel_create.argtypes = [ctypes.c_int64, ctypes.c_int64, ctypes.c_int64,
                                       ctypes.c_double, ctypes.c_double]
_lib.cf_wind_tunnel_create_from_stl.restype = ctypes.c_void_p
_lib.cf_wind_tunnel_create_from_stl.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int,
                                                ctypes.c_int, ctypes.c_int, ctypes.c_double,
                                                ctypes.c_double]
_lib.cf_wind_tunnel_set_sphere.argtypes = [ctypes.c_void_p, ctypes.c_double, ctypes.c_double,
                                           ctypes.c_double, ctypes.c_double]
_lib.cf_wind_tunnel_run.argtypes = [ctypes.c_void_p, ctypes.c_int64]
_lib.cf_wind_tunnel_destroy.argtypes = [ctypes.c_void_p]
for _n in ("nx", "ny", "nz"):
    getattr(_lib, f"cf_wind_tunnel_{_n}").restype = ctypes.c_int64
    getattr(_lib, f"cf_wind_tunnel_{_n}").argtypes = [ctypes.c_void_p]
_lib.cf_wind_tunnel_velocity.argtypes = [ctypes.c_void_p, _P]
_lib.cf_wind_tunnel_solid.argtypes = [ctypes.c_void_p, _P]
_lib.cf_wind_tunnel_write_vtk.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
_lib.cf_geometry_available.restype = ctypes.c_int


def has_geometry() -> bool:
    """True if the library was built with STL/OBJ geometry support (from_stl)."""
    return _lib.cf_geometry_available() == 1


class WindTunnel:
    """3D wind tunnel: external flow past an obstacle (D3Q19). The obstacle is a
    sphere (``set_sphere``, always available) or a voxelized STL/OBJ
    (``from_stl``, needs a geometry-enabled build). Write the result to VTK and
    open it in ParaView for streamlines."""

    def __init__(self, nx: int, ny: int, nz: int, omega: float, u_in: float):
        self._h = _lib.cf_wind_tunnel_create(nx, ny, nz, omega, u_in)
        if not self._h:
            raise ValueError("cf_wind_tunnel_create failed (nx, ny, nz must be > 0)")

    @classmethod
    def from_stl(cls, path: str, resolution: int = 32, u_in: float = 0.05,
                 omega: float = 1.8, pad_up: int = 32, pad_down: int = 96,
                 pad_lat: int = 32) -> "WindTunnel":
        """Build a tunnel sized around a voxelized STL/OBJ obstacle."""
        h = _lib.cf_wind_tunnel_create_from_stl(path.encode(), resolution, pad_up, pad_down,
                                                pad_lat, omega, u_in)
        if not h:
            if not has_geometry():
                raise RuntimeError(
                    "STL loading unavailable: rebuild with -DCYBERFLUIDS_GEOMETRY=ON "
                    "(CyberMeshGenerator).")
            raise RuntimeError(f"could not load or voxelize '{path}'")
        obj = cls.__new__(cls)
        obj._h = h
        return obj

    @staticmethod
    def omega_for_reynolds(u_in: float, l_char: float, re: float) -> float:
        """Relaxation rate for a target Reynolds number Re = u_in * l_char / nu."""
        nu = u_in * l_char / re
        return 1.0 / (3.0 * nu + 0.5)

    def set_sphere(self, cx: float, cy: float, cz: float, radius: float) -> None:
        _lib.cf_wind_tunnel_set_sphere(self._h, cx, cy, cz, radius)

    @property
    def nx(self) -> int:
        return _lib.cf_wind_tunnel_nx(self._h)

    @property
    def ny(self) -> int:
        return _lib.cf_wind_tunnel_ny(self._h)

    @property
    def nz(self) -> int:
        return _lib.cf_wind_tunnel_nz(self._h)

    def run(self, steps: int) -> None:
        _lib.cf_wind_tunnel_run(self._h, steps)

    def velocity(self) -> np.ndarray:
        """Velocity field as a NumPy array of shape (nx, ny, nz, 3)."""
        out = np.empty(self.nx * self.ny * self.nz * 3, dtype=np.float64)
        _lib.cf_wind_tunnel_velocity(self._h, out.ctypes.data_as(_P))
        return out.reshape(self.nx, self.ny, self.nz, 3)

    def solid(self) -> np.ndarray:
        """Solid-fraction field as a NumPy array of shape (nx, ny, nz)."""
        out = np.empty(self.nx * self.ny * self.nz, dtype=np.float64)
        _lib.cf_wind_tunnel_solid(self._h, out.ctypes.data_as(_P))
        return out.reshape(self.nx, self.ny, self.nz)

    def write_vtk(self, path: str) -> None:
        """Write a legacy-VTK STRUCTURED_POINTS file (velocity, speed, solid)."""
        _lib.cf_wind_tunnel_write_vtk(self._h, path.encode())

    def close(self) -> None:
        if getattr(self, "_h", None):
            _lib.cf_wind_tunnel_destroy(self._h)
            self._h = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
