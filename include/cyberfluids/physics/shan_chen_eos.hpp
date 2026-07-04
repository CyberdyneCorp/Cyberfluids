#pragma once

#include <cmath>

namespace cyberfluids {

/// Shan-Chen pseudopotential psi(rho) = rho0 (1 - exp(-rho/rho0)). Its mechanical
/// instability (|G| > 4/rho0) drives single-component liquid/vapour separation.
/// See openspec/specs/physical-models/spec.md.
template <class T>
inline T shanChenPsi(T rho, T rho0) {
    return rho0 * (T(1) - std::exp(-rho / rho0));
}

/// Shan-Chen equation-of-state pressure p = cs2 rho + (cs2 G / 2) psi(rho)^2.
template <class T>
inline T shanChenPressure(T rho, T G, T rho0, T cs2) {
    const T psi = shanChenPsi(rho, rho0);
    return cs2 * rho + T(0.5) * cs2 * G * psi * psi;
}

}  // namespace cyberfluids
