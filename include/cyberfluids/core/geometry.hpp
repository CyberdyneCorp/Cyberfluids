#pragma once

#include <array>
#include <cstdint>

namespace cyberfluids {

/// An axis-aligned index box over a lattice, with inclusive bounds. Used to
/// describe a lattice's extent and to scope operations to a sub-domain.
/// See openspec/specs/core-data-structures/spec.md.
template <int Dim>
struct Box {
    static_assert(Dim == 2 || Dim == 3, "Dim must be 2 or 3");

    std::array<std::int64_t, Dim> lo{};  // inclusive lower corner
    std::array<std::int64_t, Dim> hi{};  // inclusive upper corner

    std::int64_t extent(int axis) const { return hi[axis] - lo[axis] + 1; }

    std::int64_t size() const {
        std::int64_t s = 1;
        for (int a = 0; a < Dim; ++a) s *= extent(a);
        return s;
    }

    bool contains(const std::array<std::int64_t, Dim>& p) const {
        for (int a = 0; a < Dim; ++a)
            if (p[a] < lo[a] || p[a] > hi[a]) return false;
        return true;
    }
};

}  // namespace cyberfluids
