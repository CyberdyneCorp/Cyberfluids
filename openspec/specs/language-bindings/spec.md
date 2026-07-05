# language-bindings

## Purpose

Although Cyberfluids is a C++20 library, it SHALL be usable from **Python** and **Swift**
so simulations can be configured and driven from higher-level environments. Python bindings
SHALL interoperate with NumPy through NumPP.
## Requirements
### Requirement: Python binding
The library SHALL provide a Python binding that can be imported to build, configure, run,
and inspect simulations through the underlying C++ core.

#### Scenario: Drive a simulation from Python
- **WHEN** a Python script imports the binding, configures a lattice, and advances it
- **THEN** the simulation SHALL run through the C++ core and return results to Python

### Requirement: NumPP ↔ NumPy interop
The Python binding SHALL expose lattice fields as NumPy arrays without copying where
possible, so field data flows between the C++ NumPP tensors and Python NumPy arrays
coherently.

#### Scenario: Read a field as a NumPy array
- **WHEN** a macroscopic field is requested from Python
- **THEN** it SHALL be returned as a NumPy array sharing (or faithfully mirroring) the
  underlying NumPP tensor data

### Requirement: Swift binding
The library SHALL provide a Swift binding usable on Apple platforms (macOS/iOS/iPadOS) to
configure and run simulations through the C++ core.

#### Scenario: Drive a simulation from Swift
- **WHEN** a Swift program links the binding, configures a lattice, and advances it
- **THEN** the simulation SHALL run through the C++ core and expose results to Swift

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

### Requirement: Wind-tunnel binding
The Python binding SHALL expose a 3D wind-tunnel solver for external flow past an
obstacle, so a user can load geometry, run the simulation, and export the flow for
visualization without writing C++.

The binding SHALL allow constructing a tunnel with an analytic sphere obstacle
(available in any build) or from an STL/OBJ mesh (available when the library is
built with geometry support). It SHALL run the simulation, return the velocity and
solid-fraction fields as NumPy arrays, and write a legacy-VTK STRUCTURED_POINTS
file (velocity, speed, solid mask) openable in ParaView. When STL loading is
requested from a build without geometry support, the binding SHALL raise a clear
error directing the user to enable it.

#### Scenario: Sphere wind tunnel from Python
- **WHEN** a `WindTunnel` is created, given a sphere obstacle, and run
- **THEN** `velocity()` SHALL return a finite `(nx, ny, nz, 3)` array with ~zero
  velocity inside the solid and a momentum-deficit wake behind it

#### Scenario: Export for visualization
- **WHEN** `write_vtk(path)` is called after a run
- **THEN** a VTK STRUCTURED_POINTS file with the velocity vector, speed, and solid
  fields SHALL be produced and openable in ParaView

#### Scenario: STL loading requires a geometry build
- **WHEN** `WindTunnel.from_stl` is called on a library built without geometry support
- **THEN** it SHALL raise an error telling the user to rebuild with geometry enabled

