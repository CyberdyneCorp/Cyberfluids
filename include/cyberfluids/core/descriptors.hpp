#pragma once

#include <array>

#include "cyberfluids/core/descriptor.hpp"

/// Standard nearest-neighbour hydrodynamic lattice descriptors.
///
/// Velocity sets, weights, and sound speed match the Palabos reference
/// (cs2 = 1/3). Direction 0 is the rest population. `opposite[i]` is the index
/// whose velocity is -c[i] (used by bounce-back). See
/// openspec/specs/lattice-descriptors/spec.md.
namespace cyberfluids::descriptors {

/// D2Q9: 2D, 9 velocities.
struct D2Q9 {
    static constexpr int d = 2;
    static constexpr int q = 9;
    static constexpr int numPop = 9;
    static constexpr double cs2 = 1.0 / 3.0;
    static constexpr double invCs2 = 3.0;

    static constexpr std::array<std::array<int, 2>, 9> c = {{
        {{0, 0}},
        {{1, 0}}, {{0, 1}}, {{-1, 0}}, {{0, -1}},
        {{1, 1}}, {{-1, 1}}, {{-1, -1}}, {{1, -1}},
    }};

    static constexpr std::array<int, 9> opposite = {0, 3, 4, 1, 2, 7, 8, 5, 6};

    static constexpr std::array<int, 9> cNormSqr = {0, 1, 1, 1, 1, 2, 2, 2, 2};

    static constexpr std::array<double, 9> t = {
        4.0 / 9.0,
        1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0,
        1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
    };
};

/// D3Q19: 3D, 19 velocities (rest + 6 face + 12 edge).
struct D3Q19 {
    static constexpr int d = 3;
    static constexpr int q = 19;
    static constexpr int numPop = 19;
    static constexpr double cs2 = 1.0 / 3.0;
    static constexpr double invCs2 = 3.0;

    static constexpr std::array<std::array<int, 3>, 19> c = {{
        {{0, 0, 0}},
        {{1, 0, 0}}, {{-1, 0, 0}}, {{0, 1, 0}}, {{0, -1, 0}}, {{0, 0, 1}}, {{0, 0, -1}},
        {{1, 1, 0}}, {{-1, -1, 0}}, {{1, -1, 0}}, {{-1, 1, 0}},
        {{1, 0, 1}}, {{-1, 0, -1}}, {{1, 0, -1}}, {{-1, 0, 1}},
        {{0, 1, 1}}, {{0, -1, -1}}, {{0, 1, -1}}, {{0, -1, 1}},
    }};

    static constexpr std::array<int, 19> opposite = {
        0, 2, 1, 4, 3, 6, 5, 8, 7, 10, 9, 12, 11, 14, 13, 16, 15, 18, 17,
    };

    static constexpr std::array<int, 19> cNormSqr = {
        0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    };

    static constexpr std::array<double, 19> t = {
        1.0 / 3.0,
        1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0,
        1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
        1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
        1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
    };
};

static_assert(LatticeDescriptor<D2Q9>, "D2Q9 must satisfy LatticeDescriptor");
static_assert(LatticeDescriptor<D3Q19>, "D3Q19 must satisfy LatticeDescriptor");

}  // namespace cyberfluids::descriptors
