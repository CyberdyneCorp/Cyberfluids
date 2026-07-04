# streaming-and-timestep

## Purpose

Cyberfluids SHALL advance a lattice in time via the LBM collide-and-stream cycle. The
streaming step propagates post-collision populations along the descriptor's discrete
velocities to neighbouring cells. Iteration SHALL use clean C++20 Ranges and be
parallelizable across backends.

## Requirements

### Requirement: Collide-and-stream cycle
The library SHALL implement the LBM time step as collision followed by streaming, exposed
both as separate `collide()`/`stream()` operations and as a fused `collideAndStream()`,
producing identical macroscopic results.

#### Scenario: Fused equals separate
- **WHEN** one step is run via `collideAndStream()` and, from the same state, via
  `collide()` then `stream()`
- **THEN** the resulting population fields SHALL be identical up to floating-point rounding

### Requirement: Streaming along discrete velocities
Streaming SHALL move each post-collision population `f_i` from a cell to the neighbour
offset by the descriptor velocity `c_i`, for every direction `i`.

#### Scenario: Population propagation
- **WHEN** the streaming step runs
- **THEN** for each cell and each direction `i`, `f_i` SHALL be copied to the cell located
  at `x + c_i`

#### Scenario: Boundary-safe streaming
- **WHEN** streaming would move a population off the lattice at a domain edge
- **THEN** the step SHALL not read or write out of bounds; the edge behavior SHALL be
  determined by the boundary condition assigned there (e.g. bounce-back, periodic)

### Requirement: Range-based, parallel-ready iteration
The lattice traversal SHALL be expressed with C++20 Ranges over cell indices such that the
same loop body executes serially or in parallel depending on the active backend, without
model-code changes.

#### Scenario: Backend-agnostic iteration
- **WHEN** the same simulation is executed on the CPU backend and (later) a GPU backend
- **THEN** the collide-and-stream loop body SHALL be unchanged and SHALL produce matching
  results within tolerance
