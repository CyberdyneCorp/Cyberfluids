#pragma once

#include <concepts>

namespace cyberfluids {

/// A lattice descriptor fixes, at compile time, the DdQq stencil a Lattice
/// Boltzmann model runs on: the spatial dimension `d`, the discrete-velocity
/// count `q` (= `numPop`), the velocity set `c`, their squared norms
/// `cNormSqr`, the opposite-direction indices `opposite`, the weights `t`, and
/// the sound-speed scalars `cs2`/`invCs2`.
///
/// The Concept gates template instantiation so a non-conforming type is
/// rejected with a clear constraint error rather than a deep template error.
/// See openspec/specs/lattice-descriptors/spec.md.
template <class D>
concept LatticeDescriptor = requires {
    // Scalar stencil constants.
    { D::d }      -> std::convertible_to<int>;
    { D::q }      -> std::convertible_to<int>;
    { D::numPop } -> std::convertible_to<int>;
    { D::cs2 }    -> std::convertible_to<double>;
    { D::invCs2 } -> std::convertible_to<double>;

    // Per-direction data (indexable): velocities, their squared norms,
    // opposite-direction indices, and weights.
    D::c[0][0];
    D::cNormSqr[0];
    D::opposite[0];
    D::t[0];

    // Structural invariants (only evaluated once the above are satisfied).
    requires D::numPop == D::q;
    requires D::d >= 1 && D::d <= 3;
    requires D::q >= 1;
};

}  // namespace cyberfluids
