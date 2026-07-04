## MODIFIED Requirements

### Requirement: Checkpoint and restart
The library SHALL serialize the lattice populations (the `{q, ncells}` distribution functions —
the entire live state between steps) to a NumPy `.npy` file and restore them byte-exactly, so an
interrupted simulation resumes bit-for-bit from the last checkpoint. Loading SHALL reject a file
whose shape does not match the target lattice.

#### Scenario: Resume from checkpoint
- **WHEN** a simulation is checkpointed at step N and later restarted from that checkpoint
- **THEN** continuing to step M SHALL yield bit-identical state to an uninterrupted run to step M
  (on a deterministic backend)

#### Scenario: Round-trip fidelity
- **WHEN** populations are saved and then loaded into a matching lattice
- **THEN** the restored populations SHALL be byte-identical to the saved ones

#### Scenario: Shape mismatch rejected
- **WHEN** a checkpoint is loaded into a lattice of different `q` or `ncells`
- **THEN** the load SHALL fail rather than corrupt state
