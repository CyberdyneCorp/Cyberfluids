#pragma once

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptors.hpp"

namespace cyberfluids::boundary {

/// Zou/He velocity (Dirichlet) boundary on the TOP wall (y = ny-1) for D2Q9.
///
/// After streaming, the incoming populations at a top-wall node — those with a
/// negative y-component (indices 4, 7, 8 in descriptors::D2Q9) — are unknown.
/// Given the prescribed wall velocity (ux, uy), this closes them by the Zou/He
/// non-equilibrium-bounce-back rule. The remaining populations (0,1,2,3,5,6)
/// must already hold their post-streaming values.
///
/// Derivation for this descriptor's velocity ordering
/// (1:E 2:N 3:W 4:S 5:NE 6:NW 7:SW 8:SE):
///   rho = [f0 + f1 + f3 + 2(f2 + f5 + f6)] / (1 + uy)
///   f4  = f2 - (2/3) rho uy
///   f7  = f5 + 1/2 (f1 - f3) - 1/2 rho ux - 1/6 rho uy
///   f8  = f6 - 1/2 (f1 - f3) + 1/2 rho ux - 1/6 rho uy
/// See openspec/specs/boundary-conditions/spec.md.
template <class T>
void zouHeVelocityTop(Cell<T, descriptors::D2Q9>& cell, T ux, T uy) {
    const T f0 = cell[0], f1 = cell[1], f2 = cell[2], f3 = cell[3], f5 = cell[5], f6 = cell[6];

    const T rho = (f0 + f1 + f3 + T(2) * (f2 + f5 + f6)) / (T(1) + uy);

    cell[4] = f2 - (T(2) / T(3)) * rho * uy;
    cell[7] = f5 + T(0.5) * (f1 - f3) - T(0.5) * rho * ux - (T(1) / T(6)) * rho * uy;
    cell[8] = f6 - T(0.5) * (f1 - f3) + T(0.5) * rho * ux - (T(1) / T(6)) * rho * uy;
}

}  // namespace cyberfluids::boundary
