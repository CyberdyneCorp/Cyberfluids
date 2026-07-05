# Add zero-copy DLPack tensor interop

## Why

Lattice fields are currently copied out to NumPy (`velocity()`/`density()` build
a fresh array each call). For ML/analysis pipelines (PyTorch, JAX) this copy is
wasteful on large grids. DLPack is the standard cross-framework protocol for
sharing tensor memory with zero copy; exposing it lets consumers alias the live
NumPP buffers directly.

## What Changes

- Vendor the DLPack ABI header (`include/cyberfluids/interop/dlpack.h`) — an
  ABI spec, not a dependency — and add a header-only exporter
  `cyberfluids::interop::to_dlpack(const numpp::ndarray&)` producing a
  `DLManagedTensor` that aliases the array's buffer (CPU, correct dtype, NULL
  strides when C-contiguous) and holds its own buffer reference (borrow
  semantics: data outlives the source field for as long as the tensor lives).
- Add C-API entry points `cf_cavity2d_{populations,velocity,density}_dlpack`.
- Add an in-place macroscopic cache (`refreshMacroscopic`, `densityField`,
  `velocityField`) to the 2D cavity so velocity/density tensors alias stable
  storage (unlike populations, whose buffer is swapped each step).
- Implement the Python `__dlpack__`/`__dlpack_device__` protocol via PyCapsule
  so `numpy.from_dlpack` / `torch.from_dlpack` build zero-copy views.
- Add C++ (`test_dlpack`) and Python (`test_dlpack.py`, self-skipping without
  numpy>=1.23) tests.

## Non-goals

- GPU-device DLPack (Metal/CUDA buffers) — CPU only for now.
- DLPack export from the 3D cavity or other solvers (only the 2D cavity C-API).
- Writable-import semantics: DLPack has no writable flag, so consumers import
  read-only; zero-copy is proven by shared data pointer, not by write-through.
