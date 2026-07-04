## Context

Cyberfluids starts from an empty repository. The target capability map in
`openspec/specs/` describes the full library; this change carves the smallest slice that
(a) compiles and runs on the CPU, and (b) is provably correct against the Palabos oracle.
Everything downstream (more dynamics, GPU backends, multiphase, STL) depends on the
abstractions fixed here — the descriptor Concept, the `Dynamics` interface, the backend
seam, and the NumPP-backed storage layout — so getting these shapes right matters more
than breadth.

## Goals / Non-Goals

**Goals:**
- A CMake project that fetches NumPP/SciPP and builds a C++20 core with a CPU backend.
- D2Q9 + D3Q19 descriptors, `BlockLattice`/`Cell`, NumPP-backed SoA storage.
- BGK collision, collide-and-stream, bounce-back + Zou/He velocity boundaries.
- A 2D and 3D lid-driven cavity that matches Palabos within a documented tolerance.
- Python and Swift bindings that can build and run the cavity.
- Stable abstraction seams: descriptor Concept, `Dynamics<T,Descriptor>`, backend dispatch.

**Non-Goals:**
- TRT/MRT/regularized/forced dynamics, D3Q27, D2Q5/D3Q7 (follow-up changes).
- GPU backends (CUDA/Metal/OpenCL/SYCL) beyond compile-time flags and stub seams.
- Multiphase (Shan-Chen), thermal (Boussinesq), porous media, STL import/voxelization.
- Checkpointing and VTK output beyond what the oracle comparison needs.
- MPI / multi-node (explicitly excluded by architecture).

## Decisions

- **Storage: SoA populations in a NumPP tensor.** Layout populations as `[q][cells]`
  (component-major) so each direction's data is contiguous, favouring `par_unseq`
  vectorization over the streaming/collision loops. Alternative — AoS `Cell` objects —
  is simpler to read but defeats vectorization and GPU coalescing; rejected. `Cell` is a
  lightweight *view* over the tensor, not the owner.
- **Descriptor as a compile-time type constrained by a Concept.** Descriptors are
  stateless types with `static constexpr` members; a `LatticeDescriptor` Concept gates
  template instantiation, giving readable errors and zero runtime cost. Alternative —
  runtime-polymorphic descriptors — was rejected as it blocks inlining/vectorization.
- **Backend seam via a policy/strategy, not virtual per-cell calls.** The backend is
  selected at the loop level (who runs the kernel), not per cell, so the CPU path lowers
  to a single `std::for_each(par_unseq, ...)`. GPU backends later implement the same
  kernel-launch interface. Per-cell virtual dispatch would kill performance; rejected.
- **`Dynamics` interface stays for per-region model selection.** Collision is chosen per
  region via a `Dynamics<T,Descriptor>` interface (mirrors Palabos), but the hot BGK path
  is templated/inlinable so the common case avoids virtual overhead. This preserves
  flexibility (bounce-back vs BGK vs boundary dynamics per cell) without taxing the bulk.
- **Palabos oracle: store reference data, compare offline.** Rather than link Palabos into
  the test binary (heavy, MPI-flavoured build), run Palabos once to produce reference
  fields for each benchmark, commit/generate them, and compare in-process. Keeps the
  Cyberfluids test build free of Palabos/MPI. Record the Palabos revision used.
- **Tolerance.** Compare L2 and L∞ of velocity magnitude on the cavity centerlines and the
  full field; start with L∞ ≤ 1e-3 (lattice units) at matched steps, tightened as the
  implementation matures. The exact number is documented alongside the reference data.
- **Bindings via a thin C ABI core.** Expose a minimal C-callable surface that both the
  Python (NumPP↔NumPy) and Swift bindings wrap, avoiding two independent deep bindings.

## Risks / Trade-offs

- **NumPP/SciPP maturity/API drift** → Pin to specific revisions in CMake; wrap their types
  behind a thin internal alias header so an API change is a one-file fix.
- **`std::execution::par_unseq` support varies by stdlib** → Provide a serial fallback when
  parallel execution policies are unavailable; keep results identical.
- **Oracle mismatch from convention differences** (equilibrium order, velocity units,
  boundary placement) → Match Palabos conventions exactly (`omega=1/tau`, second-order
  equilibrium, Zou/He); document any deliberate deviation. Start with a loose tolerance and
  tighten.
- **Bindings pulling scope early** → Bindings only need to construct and step the cavity for
  this change; richer APIs are deferred.

## Migration Plan

Greenfield — no migration. Landing order: build skeleton → NumPP-backed core →
descriptors/BGK/streaming → boundaries → CPU backend wiring → cavity example → oracle
harness → bindings. Each step keeps the build green.

## Open Questions

- Which specific NumPP/SciPP revisions to pin, and do they expose the tensor slicing needed
  for SoA population access without copies?
- Preferred VTK/reference-data format for storing Palabos oracle output (raw binary vs VTK)?
- Swift binding packaging target for this change — SwiftPM module vs XCFramework?
