# Cyberfluids (Python)

Python bindings for [**Cyberfluids**](https://github.com/CyberdyneCorp/Cyberfluids) — a
zero-legacy C++20 Lattice Boltzmann CFD engine. A thin, dependency-light `ctypes` wrapper over
the Cyberfluids C ABI; macroscopic fields come back as NumPy arrays.

## Install

The wheel builds and bundles the native C ABI library for you (via
[scikit-build-core](https://scikit-build-core.readthedocs.io/) + CMake, which fetches the pinned
NumPP dependency automatically):

```bash
pip install ./bindings/python        # from a Cyberfluids checkout
```

Build requirements: a C++20 compiler, CMake ≥ 3.24, and (on Linux) `libtbb` for the
`std::execution::par_unseq` backend. Runtime: `numpy`.

## Usage

```python
import cyberfluids as cf

print(cf.version())

# 2D lid-driven cavity
sim = cf.Cavity2D(128, omega=1.6)
sim.run(2000)
u = sim.velocity()                   # NumPy array, shape (ny, nx, 2)

# 3D wind tunnel past an analytic sphere
omega = cf.WindTunnel.omega_for_reynolds(u_in=0.05, l_char=20, re=100)
tunnel = cf.WindTunnel(128, 64, 64, omega=omega, u_in=0.05)
tunnel.set_sphere(36, 32, 32, 10)
tunnel.run(5000)
tunnel.write_vtk("sphere.vtk")       # open in ParaView
```

`cf.has_geometry()` reports whether the loaded library was built with STL import
(`-DCYBERFLUIDS_GEOMETRY=ON`). To point the binding at a specific prebuilt library instead of the
bundled one, set `CYBERFLUIDS_LIBRARY=/path/to/libcyberfluids_c.so`.

See the [main README](https://github.com/CyberdyneCorp/Cyberfluids) and
[bindings docs](https://github.com/CyberdyneCorp/Cyberfluids/blob/main/bindings/README.md).
