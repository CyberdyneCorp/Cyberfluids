/// Behavioral tests for the lid-driven cavity (no oracle yet): the simulation
/// stays finite and stable, density stays near 1, mass is (nearly) conserved,
/// and the moving lid drives the expected top-vs-bottom velocity asymmetry.

#include <array>
#include <cmath>
#include <cstdint>

#include "cyberfluids/solver/lid_driven_cavity.hpp"
#include "testing.hpp"

int main() {
    // ---- 2D cavity ----
    using Cav = cyberfluids::solver::LidDrivenCavity2D<>;
    const std::int64_t n = 32;
    const double U = 0.1, omega = 1.0;
    Cav cav(n, n, omega, U);

    const double mass0 = cav.totalMass();
    cav.run(3000);

    bool finite = true;
    double umax = 0.0, rmin = 1e9, rmax = -1e9;
    for (std::int64_t x = 0; x < n; ++x)
        for (std::int64_t y = 0; y < n; ++y) {
            const double r = cav.density(x, y);
            const auto u = cav.velocity(x, y);
            if (!std::isfinite(r) || !std::isfinite(u[0]) || !std::isfinite(u[1])) finite = false;
            const double sp = std::sqrt(u[0] * u[0] + u[1] * u[1]);
            if (sp > umax) umax = sp;
            if (r < rmin) rmin = r;
            if (r > rmax) rmax = r;
        }
    CF_CHECK(finite);
    CF_CHECK(umax < 0.3);                 // stable: no blow-up (~< 3U)
    CF_CHECK(rmin > 0.7 && rmax < 1.3);   // density stays near 1
    // Moving-wall bounce-back conserves mass exactly (to rounding).
    CF_CHECK_CLOSE(cav.totalMass(), mass0, 1e-9 * mass0);

    // Lid drives flow: u_x larger near the top than near the bottom (center column).
    const double uxTop = cav.velocity(n / 2, n - 2)[0];
    const double uxBot = cav.velocity(n / 2, 1)[0];
    CF_CHECK(uxTop > 0.0);
    CF_CHECK(uxTop > uxBot);

    // ---- 3D cavity smoke test ----
    using Cav3 = cyberfluids::solver::LidDrivenCavity3D<>;
    const std::int64_t m = 12;
    Cav3 c3(m, m, m, 1.0, 0.1);
    c3.run(300);
    bool ok3 = true;
    double umax3 = 0.0;
    for (std::int64_t x = 0; x < m; ++x)
        for (std::int64_t y = 0; y < m; ++y)
            for (std::int64_t z = 0; z < m; ++z) {
                const double r = c3.density(x, y, z);
                const auto u = c3.velocity(x, y, z);
                if (!std::isfinite(r) || !std::isfinite(u[0])) ok3 = false;
                const double sp = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
                if (sp > umax3) umax3 = sp;
            }
    CF_CHECK(ok3);
    CF_CHECK(umax3 < 0.3);

    if (cftest::failures == 0) std::printf("cavity: all checks passed\n");
    return cftest::failures;
}
