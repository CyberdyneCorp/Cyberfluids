# Add GitHub Actions CI pipeline

## Why

The project has no automated build/test gate; regressions can land unnoticed.
A CI pipeline that builds and runs the full test suite on every push/PR — plus
an OpenSpec structural check — makes the green-tests + spec-validated invariant
enforced rather than manual.

## What Changes

- Add `.github/workflows/ci.yml` with three jobs on push(main)+pull_request and
  a cancel-in-progress concurrency group:
  - `build-test` (ubuntu-latest, the real gate): install cmake/ninja/libtbb-dev
    + numpy, clone NumPP/SciPP, bootstrap deps into `.deps`, configure (injecting
    `-ltbb` for libstdc++'s `par_unseq` backend, which no target links today),
    build, and run the **full** ctest suite (including the oracle tests, which
    compare against in-repo CSVs and need no Palabos at runtime).
  - `macos-build` (macos-latest): build with `-DCYBERFLUIDS_METAL=ON` (Metal is
    build-verified; the GPU test is excluded since CI VMs lack a guaranteed GPU)
    and run the Swift binding test.
  - `openspec-validate`: `openspec validate --all --strict`.

## Non-goals

- Publishing packages/artifacts or release automation.
- Adding a `find_package(TBB)`/`TBB::tbb` link in CMake (the CI injects the flag;
  a proper CMake fix is a separate change).
- CUDA/OpenCL runners (no hardware; would ship weaker-validated).

## Assumptions

- NumPP/SciPP are cloneable from CyberdyneCorp. If private, a read-scoped PAT in
  the `CYBERDYNE_DEPS_TOKEN` secret is required (the workflow uses it when set).
