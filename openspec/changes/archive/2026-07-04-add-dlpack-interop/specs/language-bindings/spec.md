# language-bindings (delta)

## ADDED Requirements

### Requirement: DLPack zero-copy interop
The library SHALL export lattice fields as DLPack tensors (the standard
cross-framework tensor-exchange ABI) without copying, so consumers such as
NumPy, PyTorch, and JAX can alias the live buffer via `from_dlpack`.

The C ABI SHALL provide functions returning a `DLManagedTensor` that aliases a
field's buffer with the correct CPU device, dtype, shape, and strides (NULL when
C-contiguous). The returned tensor SHALL hold its own reference to the underlying
buffer so the data remains valid for the tensor's lifetime (borrow semantics),
and the caller SHALL own the tensor and invoke its `deleter` exactly once.

The Python binding SHALL implement the `__dlpack__` / `__dlpack_device__`
protocol via a PyCapsule (named `dltensor`, renamed to `used_dltensor` on
consumption) so a consumer takes ownership of the deleter, avoiding double free.

#### Scenario: Zero-copy import into NumPy
- **WHEN** a lattice field is imported through `numpy.from_dlpack`
- **THEN** the resulting array SHALL share the same memory as the C++ field
  (identical data pointer), with no copy

#### Scenario: Exported tensor outlives its source field
- **WHEN** a DLManagedTensor is exported and the source field object is released
  while the tensor (or a consumer built from it) is still alive
- **THEN** the tensor's data SHALL remain valid until its deleter runs
