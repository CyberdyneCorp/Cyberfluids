# external-fields Specification

## Purpose
TBD - created by archiving change add-external-fields. Update Purpose after archive.
## Requirements
### Requirement: Per-cell external-field storage
The library SHALL provide `ExternalField<T, Descriptor>` — a Structure-of-Arrays store of
per-cell external scalars in a NumPP tensor of shape `{nExt, ncells}`, where `nExt` is the
number of external scalars the descriptor declares. It SHALL share the populations' per-cell
stride (`ncells`) so a `Cell` can address external scalars the same way it addresses
populations. External fields are **node-local**: the streaming step SHALL NOT move them.

#### Scenario: Round-trip external data
- **WHEN** a value is written to external scalar `s` of cell `c` and later read back
- **THEN** it SHALL return the written value, addressed at offset `s*ncells + c`

#### Scenario: Streaming leaves external data in place
- **WHEN** a lattice with external fields is advanced by collide-and-stream
- **THEN** the external field values SHALL remain attached to their original cells (not shifted)

### Requirement: Zero-cost when no external fields are declared
A descriptor that declares no external fields SHALL incur no per-cell external storage and no
allocation, and a `Cell` from such a lattice SHALL report no external data.

#### Scenario: Plain descriptor has no external storage
- **WHEN** a lattice uses a base descriptor (e.g. D2Q9) with no external fields
- **THEN** its `ExternalField` SHALL allocate nothing and expose a null external origin per cell

### Requirement: Descriptor external-field opt-in
The library SHALL let a descriptor declare external fields compositionally (e.g. a force or an
advection velocity) via a wrapper over a base descriptor, WITHOUT modifying the base descriptor
or the `LatticeDescriptor` concept. Whether a descriptor declares external fields SHALL be
detected at compile time.

#### Scenario: Forced/advected descriptor variants
- **WHEN** a forced or advected descriptor variant is formed from a base stencil
- **THEN** it SHALL satisfy `LatticeDescriptor`, expose the declared external-scalar count, and
  the underlying base descriptor SHALL remain unchanged

### Requirement: Cell external-field access
A `Cell` SHALL expose read/write access to its external scalars by offset, without changing the
population access API (`operator[]`) or requiring external storage to exist.

#### Scenario: Read a per-cell external scalar
- **WHEN** `cell.external(offset)` is read or written on a lattice whose descriptor declares
  external fields
- **THEN** it SHALL address that cell's external scalar in the backing `ExternalField`

### Requirement: One-way fluid-to-scalar coupling
The library SHALL provide a backend-dispatched operation that writes a fluid lattice's velocity
into the external velocity slots of an advection-diffusion lattice of matching geometry, so the
scalar is advected by the fluid.

#### Scenario: Copy fluid velocity into the AD external field
- **WHEN** the coupling runs over two lattices with equal extents
- **THEN** each AD cell's external velocity SHALL equal the fluid velocity at the same location

#### Scenario: Geometry mismatch is rejected
- **WHEN** the coupling is invoked on lattices with differing extents
- **THEN** it SHALL fail (assert) rather than read or write out of bounds

### Requirement: Two-way buoyancy coupling
The library SHALL provide a backend-dispatched operation that writes a temperature-derived
buoyancy force into a fluid lattice's per-cell external force field, `F = rho·g·β·(T − T_ref)`
along a configurable gravity axis (other components zero), where `T` is the AD scalar at the
matching cell. It SHALL share the geometry-mismatch and unassigned-cell handling of the one-way
coupling, and SHALL leave the one-way `copyVelocityToExternal` unchanged.

#### Scenario: Buoyancy force written from temperature
- **WHEN** the coupling runs over a fluid lattice and a matching temperature lattice
- **THEN** each fluid cell's external force along the gravity axis SHALL equal
  `rho·g·β·(T − T_ref)` at that cell, and the other force components SHALL be zero

#### Scenario: Geometry mismatch is rejected
- **WHEN** the coupling is invoked on lattices with differing extents
- **THEN** it SHALL fail rather than read or write out of bounds

