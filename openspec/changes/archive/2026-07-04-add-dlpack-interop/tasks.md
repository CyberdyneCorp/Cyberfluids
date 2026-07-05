# Tasks

## 1. Exporter
- [x] 1.1 Vendor `include/cyberfluids/interop/dlpack.h` (guarded ABI header)
- [x] 1.2 Header-only `cyberfluids::interop::to_dlpack` with borrow-semantics
      deleter + dtype mapping + NULL/element strides

## 2. C ABI
- [x] 2.1 Declare + implement `cf_cavity2d_{populations,velocity,density}_dlpack`
- [x] 2.2 2D cavity in-place macroscopic cache (refreshMacroscopic + accessors)

## 3. Python
- [x] 3.1 ctypes DLPack structs + PyCapsule protocol (raw-pointer callback to
      avoid the py_object refcount-steal crash) + `_DLPackField`
- [x] 3.2 `Cavity2D.populations()/velocity_field()/density_field()`

## 4. Validation
- [x] 4.1 `tests/test_dlpack.cpp`: device/dtype/shape/strides, zero-copy
      write-through, borrow-survives-deleter, strided export
- [x] 4.2 `bindings/python/test_dlpack.py`: zero-copy via data-pointer equality
      (numpy, optional torch); self-skips without numpy>=1.23
- [x] 4.3 Register both in CMake; full suite passes (31/31)

## 5. Spec & review
- [x] 5.1 Add DLPack requirement to language-bindings spec
- [x] 5.2 `openspec validate --all --strict`
- [x] 5.3 Adversarial review: fixed exception-safety leak in to_dlpack (compute dtype before alloc) + regression test
- [x] 5.4 Archive
