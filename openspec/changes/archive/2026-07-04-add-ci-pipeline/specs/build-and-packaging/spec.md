# build-and-packaging (delta)

## ADDED Requirements

### Requirement: Continuous integration
The repository SHALL provide a GitHub Actions workflow that builds the project
and runs the test suite on every push to `main` and every pull request, and that
validates the OpenSpec artifacts.

The Linux job SHALL build with GCC (exercising the `std::execution::par_unseq`
CPU backend, linking libstdc++'s parallel backend) and run the full CTest suite,
including the oracle tests (which compare against in-repo reference CSVs and
require no external solver at runtime). A separate OpenSpec job SHALL run
`openspec validate --all --strict`.

#### Scenario: Pull request gate
- **WHEN** a pull request is opened or updated
- **THEN** CI SHALL build the project and run the test suite, and the checks SHALL
  fail if any test fails or if `openspec validate --all --strict` fails

#### Scenario: Superseded runs are cancelled
- **WHEN** a new commit is pushed to a branch with a run already in progress
- **THEN** the in-progress run SHALL be cancelled (concurrency group)
