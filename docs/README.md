# Cyberfluids Documentation

Friendly, task-oriented docs for Cyberfluids. The **authoritative contract** for every
capability lives in the OpenSpec specs under [`../openspec/specs/`](../openspec/specs) —
these pages summarize and link to them.

> **Status: alpha.** The foundational MVP is implemented and Palabos-validated (12/12 tests).
> See the [feature table](../README.md#-feature-status) for what's done vs planned; the MVP is
> the [`bootstrap-cyberfluids-core`](../openspec/changes/bootstrap-cyberfluids-core) change.

## Contents

- [Getting started](getting-started.md) — build the library, resolve NumPP/SciPP, run the oracle tests.
- [Architecture](architecture.md) — layers, abstraction seams, and data flow.
- [Features](features.md) — capability-by-capability overview and status.
- [Backends](backends.md) — CPU and GPU backends; how switching works.
- [Oracle validation](oracle-validation.md) — using Palabos as a numerical oracle.
- [Roadmap](roadmap.md) — milestones after the MVP.

## The spec is the source of truth

| Layer | Where |
|---|---|
| Project context | [`../openspec/project.md`](../openspec/project.md) |
| Target capability specs | [`../openspec/specs/`](../openspec/specs) |
| In-flight changes | [`../openspec/changes/`](../openspec/changes) |

To propose a change, use the OpenSpec workflow (`/opsx:propose`). See [openspec.dev](https://openspec.dev).
