# oracle-validation Specification

## Purpose
TBD - created by archiving change bootstrap-cyberfluids-core. Update Purpose after archive.
## Requirements
### Requirement: Palabos as numerical oracle
The test suite SHALL validate Cyberfluids against Palabos as a numerical oracle: for a given
benchmark, identical physical setups (geometry, Reynolds number, `omega`, resolution, and step
count) SHALL be run in both codes and their macroscopic fields compared. This SHALL cover both the
**2D (D2Q9)** and **3D (D3Q19)** lid-driven cavities. Because the two codes use different wall
boundary schemes, the comparison is physics-level over interior nodes within a documented tolerance.

#### Scenario: Matching 2D cavity result
- **WHEN** a 2D lid-driven cavity is run in Cyberfluids and in Palabos with the same parameters
- **THEN** the interior centerline velocity SHALL agree within the documented tolerance

#### Scenario: Matching 3D cavity result
- **WHEN** a 3D (D3Q19) lid-driven cavity is run in Cyberfluids and in Palabos with the same
  parameters
- **THEN** the interior centerline velocity SHALL agree within the documented tolerance (the peak
  deviation being the near-lid node where the wall schemes differ)

### Requirement: Documented tolerance and reference data
The oracle comparison SHALL use an explicit, documented tolerance (absolute and/or relative)
appropriate to the benchmark, and SHALL compare against reference data produced by Palabos
(precomputed and stored, or generated in the test).

#### Scenario: Tolerance is explicit and enforced
- **WHEN** an oracle regression test runs
- **THEN** it SHALL fail if the field deviation exceeds the documented tolerance, and pass
  otherwise, reporting the observed maximum deviation

### Requirement: Oracle harness reproducibility
The oracle harness SHALL make comparisons reproducible: parameters, tolerances, and the
Palabos reference version SHALL be recorded so a failure can be attributed to a specific
change.

#### Scenario: Reproduce a comparison
- **WHEN** an oracle test is re-run with the same inputs
- **THEN** it SHALL produce the same pass/fail verdict and the same reported deviation up to
  floating-point determinism of the backends

