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
