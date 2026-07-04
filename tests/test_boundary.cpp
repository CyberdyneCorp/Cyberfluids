/// Verifies boundary conditions: Zou/He self-consistency (imposed velocity and
/// density are recovered from the closed populations) and the bounce-back
/// helpers (reflection + moving-wall momentum term).

#include <array>
#include <cstdint>

#include "cyberfluids/boundary/bounce_back.hpp"
#include "cyberfluids/boundary/zou_he.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::D2Q9;

int main() {
    // Zou/He: after closing the unknowns, the moments equal the imposed (ux, uy).
    {
        cyberfluids::BlockLattice2D<double, D2Q9> lat(1, 1);
        auto cell = lat.get(0, 0);
        cell[0] = 0.40; cell[1] = 0.11; cell[2] = 0.09;
        cell[3] = 0.07; cell[5] = 0.03; cell[6] = 0.02;
        cell[4] = cell[7] = cell[8] = 0.0;

        const double ux = 0.1, uy = -0.05;
        cyberfluids::boundary::zouHeVelocityTop<double>(cell, ux, uy);

        double rho = 0.0, jx = 0.0, jy = 0.0;
        for (int i = 0; i < 9; ++i) {
            rho += cell[i];
            jx += cell[i] * D2Q9::c[i][0];
            jy += cell[i] * D2Q9::c[i][1];
        }
        CF_CHECK_CLOSE(jx / rho, ux, 1e-12);
        CF_CHECK_CLOSE(jy / rho, uy, 1e-12);
        const double rhoExpected = (0.40 + 0.11 + 0.07 + 2 * (0.09 + 0.03 + 0.02)) / (1.0 + uy);
        CF_CHECK_CLOSE(rho, rhoExpected, 1e-12);
    }

    // Bounce-back helpers.
    {
        cyberfluids::PopulationField<double, D2Q9> p(1);
        for (int i = 0; i < 9; ++i) p(i, 0) = i + 1;
        for (int i = 0; i < 9; ++i)
            CF_CHECK(cyberfluids::boundary::noSlipReflected(p, i, 0) == p(D2Q9::opposite[i], 0));

        const std::array<double, 2> uw{0.1, 0.0};
        // dir 1 = (1,0): +2 t1 rho invCs2 (c.u) = (2/3) rho U.
        CF_CHECK_CLOSE((cyberfluids::boundary::movingWallTerm<double, D2Q9, 2>(1, 1.0, uw)),
                       (2.0 / 3.0) * 0.1, 1e-14);
        // dir 3 = (-1,0): opposite sign.
        CF_CHECK_CLOSE((cyberfluids::boundary::movingWallTerm<double, D2Q9, 2>(3, 1.0, uw)),
                       -(2.0 / 3.0) * 0.1, 1e-14);
    }

    if (cftest::failures == 0) std::printf("boundary: all checks passed\n");
    return cftest::failures;
}
