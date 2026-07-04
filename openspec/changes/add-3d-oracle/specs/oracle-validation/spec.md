## MODIFIED Requirements

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
