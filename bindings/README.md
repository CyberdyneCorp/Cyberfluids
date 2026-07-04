# Cyberfluids language bindings

Python and Swift bindings, both thin wrappers over the same **C ABI**
(`include/cyberfluids/capi.h`, built as the shared library `cyberfluids_c`). See
[`../openspec/specs/language-bindings/spec.md`](../openspec/specs/language-bindings/spec.md).

Build the C ABI library first:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --target cyberfluids_c -j
# -> build/libcyberfluids_c.dylib (or .so)
```

## Python (ctypes + NumPy)

Pure-Python `ctypes` wrapper; macroscopic fields are returned as NumPy arrays that mirror
the NumPP-backed C++ fields.

```python
import cyberfluids as cf          # PYTHONPATH=bindings/python
cav = cf.Cavity2D(128, 128, omega=1.0, lid_velocity=0.1)
cav.run(20000)
u = cav.velocity()                # np.ndarray, shape (128, 128, 2)
rho = cav.density()               # np.ndarray, shape (128, 128)
```

The library is located via `$CYBERFLUIDS_LIBRARY`, then the repo `build/` tree, then the
system loader. Install the package with `pip install bindings/python` (runtime still needs
the built `cyberfluids_c` library on one of those paths).

## Swift (SwiftPM)

`Cyberfluids` module wrapping the C ABI via a system-library target. Build/run needs the C
header include path and the library search + rpath:

```bash
swift run --package-path bindings/swift \
  -Xcc -I"$PWD/include" \
  -Xlinker -L"$PWD/build" \
  -Xlinker -rpath -Xlinker "$PWD/build" \
  -Xlinker -rpath -Xlinker "$PWD/.deps/lib" \
  cavity-demo
```

```swift
import Cyberfluids
let cav = Cavity2D(nx: 128, ny: 128, omega: 1.0, lidVelocity: 0.1)!
cav.run(steps: 20000)
let u = cav.velocity()            // [Double], length nx*ny*2, row-major
```

## Smoke tests

Both are registered with CTest (when the toolchains are present) and build+run a cavity,
read the velocity field, and assert the lid-driven flow is physical:

```bash
ctest --test-dir build -R "python_binding|swift_binding" --output-on-failure
```
