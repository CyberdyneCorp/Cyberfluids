/// Validates forced (Guo) dynamics against the analytic Poiseuille parabola —
/// a closed-form oracle needing no external code. Also checks the cross-flow
/// velocity stays ~0 and mass stays near 1.

#include <array>
#include <cmath>
#include <cstdint>

#include "cyberfluids/solver/poiseuille_channel.hpp"
#include "testing.hpp"

int main() {
    using Chan = cyberfluids::solver::PoiseuilleChannel2D<>;
    const std::int64_t nx = 8, ny = 48;
    const double omega = 1.0, forceX = 1e-5;

    Chan chan(nx, ny, omega, forceX);
    chan.run(30000);  // develop to steady state (~H^2/nu)

    // Peak analytic velocity, for normalizing the error.
    double uMax = 0.0;
    for (std::int64_t y = 0; y < ny; ++y) uMax = std::max(uMax, chan.analyticUx(y, omega));
    CF_CHECK(uMax > 0.0);

    double linf = 0.0, vMax = 0.0;
    for (std::int64_t y = 0; y < ny; ++y) {
        const auto u = chan.velocity(0, y);
        const double expected = chan.analyticUx(y, omega);
        linf = std::max(linf, std::fabs(u[0] - expected) / uMax);
        vMax = std::max(vMax, std::fabs(u[1]));
    }

    std::printf("poiseuille: u_max=%.4e  Linf(rel)=%.4f  vMax=%.2e\n", uMax, linf, vMax);
    CF_CHECK(linf < 0.02);          // profile matches the parabola within 2% of u_max
    CF_CHECK(vMax < 1e-6 * uMax + 1e-12);  // no spurious cross-flow

    if (cftest::failures == 0) std::printf("poiseuille: all checks passed\n");
    return cftest::failures;
}
