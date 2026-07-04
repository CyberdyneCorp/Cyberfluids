## Why

Long GPU/multiphase runs need to survive interruption and resume exactly. NumPP already ships
`.npy` IO, so serializing the population tensor is a small, low-risk addition — and the
populations are the entire live lattice state between steps (the stream scratch is transient and
the collision models are stateless), so a faithful checkpoint is just that array.

## What Changes

- Add `io/checkpoint.hpp`: `saveCheckpoint(path, PopulationField)` / `loadCheckpoint(path, PopulationField)`
  over NumPP `.npy` (byte-exact round trip); `loadCheckpoint` throws on a shape mismatch.
- Add a `lattice()` accessor to `LidDrivenCavity2D`/`3D` so a solver's populations can be
  checkpointed.
- Validate: byte-exact round trip; mismatched-grid load rejected; and restart equivalence —
  running N steps + checkpoint + M more is **bit-identical** to loading that checkpoint fresh and
  running M.

## Capabilities

### Modified Capabilities
- `geometry-and-io`: the **Checkpoint and restart** requirement is made concrete (populations
  serialized via NumPy `.npy`; populations are the full restart state).

## Impact

- New `io/checkpoint.hpp`; small `lattice()` accessors on the cavity solvers; new test. Reuses
  NumPP `.npy`; no new deps, no core changes.

## Non-Goals

- Serializing a persistent, externally-set body force (all current solvers recompute the external
  force each step, so populations alone are faithful); multi-array `.npz` bundles; JSON metadata.
