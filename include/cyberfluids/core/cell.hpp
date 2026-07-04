#pragma once

#include <cstdint>

#include "cyberfluids/core/descriptor.hpp"

namespace cyberfluids {

template <class T, LatticeDescriptor Descriptor>
class Dynamics;  // forward declaration; a Cell stores only a non-owning pointer

/// A lightweight view over the populations of a single lattice cell.
///
/// Populations are stored Structure-of-Arrays (see PopulationField), so a
/// cell's `q` distribution functions are `stride` apart in memory. The Cell
/// holds the address of its `f_0` and the stride, plus a non-owning pointer to
/// its Dynamics. The lattice — not the cell — owns the storage and dynamics.
/// See openspec/specs/core-data-structures/spec.md.
template <class T, LatticeDescriptor Descriptor>
class Cell {
public:
    static constexpr int q = Descriptor::q;

    Cell(T* origin, std::int64_t stride, Dynamics<T, Descriptor>* dynamics,
         T* external = nullptr) noexcept
        : origin_(origin), stride_(stride), dynamics_(dynamics), external_(external) {}

    /// Distribution function f_iPop (0 <= iPop < q).
    T& operator[](int iPop) {
        return origin_[static_cast<std::int64_t>(iPop) * stride_];
    }
    const T& operator[](int iPop) const {
        return origin_[static_cast<std::int64_t>(iPop) * stride_];
    }

    /// Non-owning pointer to the cell's collision model (may be null).
    Dynamics<T, Descriptor>* dynamics() const noexcept { return dynamics_; }

    /// Per-cell external scalar at `offset` (shares the population stride).
    /// Only valid when the descriptor declares external fields (hasExternal()).
    T& external(int offset) {
        return external_[static_cast<std::int64_t>(offset) * stride_];
    }
    const T& external(int offset) const {
        return external_[static_cast<std::int64_t>(offset) * stride_];
    }
    bool hasExternal() const noexcept { return external_ != nullptr; }

private:
    T* origin_;
    std::int64_t stride_;
    Dynamics<T, Descriptor>* dynamics_;
    T* external_;
};

}  // namespace cyberfluids
