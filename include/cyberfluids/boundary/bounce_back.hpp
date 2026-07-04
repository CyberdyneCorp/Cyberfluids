#pragma once

#include <array>
#include <cstdint>

#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/populations.hpp"

namespace cyberfluids::boundary {

/// Halfway bounce-back (no-slip): the population that reflects into direction
/// `i` at a solid wall is the opposite post-collision population at the same
/// node. See openspec/specs/boundary-conditions/spec.md.
template <class T, LatticeDescriptor Descriptor>
T noSlipReflected(const PopulationField<T, Descriptor>& postCollision, int i, std::int64_t c) {
    return postCollision(Descriptor::opposite[i], c);
}

/// Anti-bounce-back scalar Dirichlet wall for advection-diffusion lattices:
/// imposes a fixed scalar `phiWall` (e.g. a plate temperature) at the half-node
/// wall. The reflected population is `-f_opp(i)^post + 2 t_i phiWall`.
template <class T, LatticeDescriptor Descriptor>
T antiBounceBackScalar(const PopulationField<T, Descriptor>& postCollision, int i, std::int64_t c,
                       T phiWall) {
    return -postCollision(Descriptor::opposite[i], c) +
           T(2) * static_cast<T>(Descriptor::t[i]) * phiWall;
}

/// Moving-wall correction added to the reflected population for a wall moving
/// at velocity `uWall`: +2 w_i rho (c_i . uWall) / cs2. Enforces the wall
/// velocity as a tangential no-slip (Ladd/bounce-back with momentum).
template <class T, LatticeDescriptor Descriptor, int Dim>
T movingWallTerm(int i, T rho, const std::array<T, Dim>& uWall) {
    T ciu = T(0);
    for (int a = 0; a < Dim; ++a) ciu += static_cast<T>(Descriptor::c[i][a]) * uWall[a];
    return T(2) * static_cast<T>(Descriptor::t[i]) * rho *
           static_cast<T>(Descriptor::invCs2) * ciu;
}

}  // namespace cyberfluids::boundary
