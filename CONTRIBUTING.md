# Contributing to Cyberfluids

Thanks for your interest in Cyberfluids — a zero-legacy C++20 Lattice Boltzmann CFD engine.
This guide covers how to build, the workflow we follow, and what a mergeable change looks like.

## Getting set up

```bash
git clone https://github.com/CyberdyneCorp/Cyberfluids.git
cd Cyberfluids
just bootstrap      # build + install NumPP into .deps/  (or: scripts/bootstrap_deps.sh)
just test           # configure + build + full CTest suite (CPU backend)
```

No `just`? The equivalent is `scripts/bootstrap_deps.sh` then `cmake -B build` +
`cmake --build build` + `ctest --test-dir build`. See
[docs/getting-started.md](docs/getting-started.md). GPU work: `just gpu-detect`, then
`just cuda` / `just opencl` / `just metal`.

## Development workflow

We develop spec-first with [OpenSpec](https://github.com/Fission-AI/OpenSpec). Living
capability specs are in [`openspec/specs/`](openspec/specs); completed changes are archived
under [`openspec/changes/archive/`](openspec/changes/archive).

- **Medium or large features** (new physics, backends, bindings, build/packaging behavior):
  start with an OpenSpec change (proposal → specs/tasks) before implementing. Run
  `openspec validate --all --strict` — CI enforces it.
- **Small fixes and docs:** a direct PR is fine. Still keep the specs and docs truthful — if
  your change makes a spec or doc statement false, update it in the same PR.

## What a mergeable PR looks like

- **Tests.** New behavior ships with tests. **Every bug fix includes a regression test** that
  fails before the fix and passes after.
- **Green CI.** Linux (GCC, `par_unseq` + oracle suite), macOS (Metal build + Swift binding),
  and OpenSpec validation must all pass.
- **Docs/specs in sync.** Update `docs/` and `openspec/specs/` when your change affects
  documented behavior. Don't leave a claim that the code contradicts.
- **Readable, low-complexity code.** Match the surrounding style (Concepts, Ranges, smart
  pointers, contiguous data — no raw owning pointers, no legacy macros). Keep per-function
  cognitive complexity modest; isolate genuinely irreducible algorithms (kernels, AST-like
  traversals) and flag them rather than mangling them to hit a number.
- **A descriptive PR message.** Explain what changed and why. If you found a bug, describe how
  it reproduced.

## Commit & PR style

- Concise, technical, imperative commit subjects (e.g. "Fix Shan-Chen weight formula in docs").
- Reference the OpenSpec change or issue when there is one.
- Keep unrelated changes in separate PRs.

## Numerical correctness

Physics changes are validated against **Palabos** as a numerical oracle (see
[docs/oracle-validation.md](docs/oracle-validation.md)) and GPU paths against the fp64 CPU
solver within a documented fp32 tolerance. If you change a collision model, boundary scheme, or
equilibrium, make sure the relevant oracle/GPU-vs-CPU tests still pass — and add one if a gap
exists.

## Reporting bugs & requesting features

Open an issue using the templates. For **security** issues, do **not** open a public issue —
see [SECURITY.md](SECURITY.md).

## License

By contributing, you agree that your contributions are licensed under the project's
[MIT License](LICENSE).
