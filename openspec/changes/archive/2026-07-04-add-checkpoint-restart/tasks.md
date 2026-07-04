## 1. Checkpoint API

- [x] 1.1 Add `io/checkpoint.hpp`: `saveCheckpoint`/`loadCheckpoint` over `PopulationField` via `numpp::save`/`numpp::load`; shape-mismatch throws
- [x] 1.2 Add `lattice()` accessor to `LidDrivenCavity2D`/`3D`

## 2. Validation

- [x] 2.1 Test `test_checkpoint`: byte-exact round trip; mismatched grid rejected; restart equivalence (continue == reload) bit-identical
- [x] 2.2 CTest wiring; suite green
- [x] 2.3 Adversarial review + docs update (pending)
