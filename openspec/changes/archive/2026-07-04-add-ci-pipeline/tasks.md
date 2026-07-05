# Tasks

## 1. Workflow
- [x] 1.1 `.github/workflows/ci.yml`: build-test (Linux), macos-build, openspec-validate
- [x] 1.2 Concurrency group + cancel-in-progress
- [x] 1.3 `-ltbb` linker injection (Linux GCC par_unseq); full ctest incl. oracle
- [x] 1.4 Optional CYBERDYNE_DEPS_TOKEN for sibling-repo clones

## 2. Validation
- [x] 2.1 YAML lint clean; jobs parse
- [x] 2.2 Local equivalents verified (bootstrap + cmake + full ctest green;
      `openspec validate --all --strict` green). NOTE: the Actions runner itself
      cannot be executed locally — first live signal is the PR check run.

## 3. Spec & review
- [x] 3.1 Add CI requirement to build-and-packaging spec
- [x] 3.2 Adversarial review (clean — no confirmed findings)
- [x] 3.3 Archive
