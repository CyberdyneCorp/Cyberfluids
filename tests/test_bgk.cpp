/// Verifies BGK dynamics: viscosity relation, equilibrium moment recovery,
/// equilibrium as a collision fixed point, and mass/momentum conservation.

#include <array>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::D2Q9;
using BGK = cyberfluids::BGKdynamics<double, D2Q9>;

int main() {
    const double omega = 1.0 / 0.6;
    BGK bgk(omega);

    // nu = cs2 (1/omega - 1/2).
    CF_CHECK_CLOSE(BGK::kinematicViscosity(omega), (1.0 / 3.0) * (0.6 - 0.5), 1e-15);

    using Lattice = cyberfluids::BlockLattice2D<double, D2Q9>;
    Lattice lat(1, 1);
    auto cell = lat.get(0, 0);

    // Initialize at equilibrium and recover the moments.
    const double rho0 = 1.2;
    const std::array<double, 2> u0{0.05, -0.03};
    const double uSqr = u0[0] * u0[0] + u0[1] * u0[1];
    for (int i = 0; i < 9; ++i) cell[i] = BGK::equilibrium(i, rho0, u0, uSqr);

    CF_CHECK_CLOSE(bgk.computeDensity(cell), rho0, 1e-12);
    std::array<double, 2> u{};
    bgk.computeVelocity(cell, u);
    CF_CHECK_CLOSE(u[0], u0[0], 1e-12);
    CF_CHECK_CLOSE(u[1], u0[1], 1e-12);

    // Collision at equilibrium is a fixed point.
    double before[9];
    for (int i = 0; i < 9; ++i) before[i] = cell[i];
    bgk.collide(cell);
    for (int i = 0; i < 9; ++i) CF_CHECK_CLOSE(cell[i], before[i], 1e-12);

    // Collision conserves mass and momentum from an arbitrary (non-equilibrium) state.
    for (int i = 0; i < 9; ++i) cell[i] = 0.1 + 0.01 * i;
    const double mass0 = bgk.computeDensity(cell);
    std::array<double, 2> vel0{};
    bgk.computeVelocity(cell, vel0);
    bgk.collide(cell);
    CF_CHECK_CLOSE(bgk.computeDensity(cell), mass0, 1e-12);
    std::array<double, 2> vel1{};
    bgk.computeVelocity(cell, vel1);
    CF_CHECK_CLOSE(vel1[0], vel0[0], 1e-12);
    CF_CHECK_CLOSE(vel1[1], vel0[1], 1e-12);

    if (cftest::failures == 0) std::printf("bgk: all checks passed\n");
    return cftest::failures;
}
