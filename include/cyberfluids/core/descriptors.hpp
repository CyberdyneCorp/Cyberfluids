#pragma once

#include <array>

#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/external_traits.hpp"

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

/// D3Q27: 3D, 27 velocities (rest + 6 face + 12 edge + 8 corner). Better
/// rotational isotropy than D3Q19.
struct D3Q27 {
    static constexpr int d = 3;
    static constexpr int q = 27;
    static constexpr int numPop = 27;
    static constexpr double cs2 = 1.0 / 3.0;
    static constexpr double invCs2 = 3.0;

    static constexpr std::array<std::array<int, 3>, 27> c = {{
        {{0, 0, 0}},
        {{1, 0, 0}}, {{-1, 0, 0}}, {{0, 1, 0}}, {{0, -1, 0}}, {{0, 0, 1}}, {{0, 0, -1}},
        {{1, 1, 0}}, {{-1, -1, 0}}, {{1, -1, 0}}, {{-1, 1, 0}},
        {{1, 0, 1}}, {{-1, 0, -1}}, {{1, 0, -1}}, {{-1, 0, 1}},
        {{0, 1, 1}}, {{0, -1, -1}}, {{0, 1, -1}}, {{0, -1, 1}},
        {{1, 1, 1}}, {{-1, -1, -1}}, {{1, 1, -1}}, {{-1, -1, 1}},
        {{1, -1, 1}}, {{-1, 1, -1}}, {{-1, 1, 1}}, {{1, -1, -1}},
    }};

    static constexpr std::array<int, 27> opposite = {
        0, 2, 1, 4, 3, 6, 5, 8, 7, 10, 9, 12, 11, 14, 13,
        16, 15, 18, 17, 20, 19, 22, 21, 24, 23, 26, 25,
    };

    static constexpr std::array<int, 27> cNormSqr = {
        0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    };

    static constexpr std::array<double, 27> t = {
        8.0 / 27.0,
        2.0 / 27.0, 2.0 / 27.0, 2.0 / 27.0, 2.0 / 27.0, 2.0 / 27.0, 2.0 / 27.0,
        1.0 / 54.0, 1.0 / 54.0, 1.0 / 54.0, 1.0 / 54.0, 1.0 / 54.0, 1.0 / 54.0,
        1.0 / 54.0, 1.0 / 54.0, 1.0 / 54.0, 1.0 / 54.0, 1.0 / 54.0, 1.0 / 54.0,
        1.0 / 216.0, 1.0 / 216.0, 1.0 / 216.0, 1.0 / 216.0,
        1.0 / 216.0, 1.0 / 216.0, 1.0 / 216.0, 1.0 / 216.0,
    };
};

/// D2Q5: 2D, 5 velocities — reduced stencil for scalar advection-diffusion
/// (cs2 = 1/3).
struct D2Q5 {
    static constexpr int d = 2;
    static constexpr int q = 5;
    static constexpr int numPop = 5;
    static constexpr double cs2 = 1.0 / 3.0;
    static constexpr double invCs2 = 3.0;

    static constexpr std::array<std::array<int, 2>, 5> c = {{
        {{0, 0}}, {{1, 0}}, {{-1, 0}}, {{0, 1}}, {{0, -1}},
    }};
    static constexpr std::array<int, 5> opposite = {0, 2, 1, 4, 3};
    static constexpr std::array<int, 5> cNormSqr = {0, 1, 1, 1, 1};
    static constexpr std::array<double, 5> t = {
        1.0 / 3.0, 1.0 / 6.0, 1.0 / 6.0, 1.0 / 6.0, 1.0 / 6.0,
    };
};

/// D3Q7: 3D, 7 velocities — reduced stencil for scalar advection-diffusion
/// (cs2 = 1/4, so the rest weight stays positive).
struct D3Q7 {
    static constexpr int d = 3;
    static constexpr int q = 7;
    static constexpr int numPop = 7;
    static constexpr double cs2 = 1.0 / 4.0;
    static constexpr double invCs2 = 4.0;

    static constexpr std::array<std::array<int, 3>, 7> c = {{
        {{0, 0, 0}}, {{1, 0, 0}}, {{-1, 0, 0}}, {{0, 1, 0}}, {{0, -1, 0}}, {{0, 0, 1}}, {{0, 0, -1}},
    }};
    static constexpr std::array<int, 7> opposite = {0, 2, 1, 4, 3, 6, 5};
    static constexpr std::array<int, 7> cNormSqr = {0, 1, 1, 1, 1, 1, 1};
    static constexpr std::array<double, 7> t = {
        1.0 / 4.0, 1.0 / 8.0, 1.0 / 8.0, 1.0 / 8.0, 1.0 / 8.0, 1.0 / 8.0, 1.0 / 8.0,
    };
};

static_assert(LatticeDescriptor<D2Q9>, "D2Q9 must satisfy LatticeDescriptor");
static_assert(LatticeDescriptor<D3Q19>, "D3Q19 must satisfy LatticeDescriptor");
static_assert(LatticeDescriptor<D3Q27>, "D3Q27 must satisfy LatticeDescriptor");
static_assert(LatticeDescriptor<D2Q5>, "D2Q5 must satisfy LatticeDescriptor");
static_assert(LatticeDescriptor<D3Q7>, "D3Q7 must satisfy LatticeDescriptor");

// External-field descriptor variants: a per-cell body force (Forced*) or a
// per-cell advection velocity (Advected*), composed onto a base stencil. The
// base descriptors above stay unchanged. See core/external_traits.hpp.
using ForcedD2Q9 = WithExternal<D2Q9, ext::Force<2>>;
using ForcedD3Q19 = WithExternal<D3Q19, ext::Force<3>>;
using ForcedD3Q27 = WithExternal<D3Q27, ext::Force<3>>;
using AdvectedD2Q5 = WithExternal<D2Q5, ext::Velocity<2>>;
using AdvectedD3Q7 = WithExternal<D3Q7, ext::Velocity<3>>;

static_assert(LatticeDescriptor<ForcedD2Q9>);
static_assert(LatticeDescriptor<ForcedD3Q19>);
static_assert(LatticeDescriptor<ForcedD3Q27>);
static_assert(LatticeDescriptor<AdvectedD2Q5>);
static_assert(LatticeDescriptor<AdvectedD3Q7>);
static_assert(numExternalScalars<D2Q9>() == 0);
static_assert(numExternalScalars<ForcedD2Q9>() == 2);
static_assert(numExternalScalars<AdvectedD3Q7>() == 3);

}  // namespace cyberfluids::descriptors
