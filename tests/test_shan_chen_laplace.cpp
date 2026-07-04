/// Laplace-law validation of Shan-Chen surface tension: for a droplet of radius
/// R at equilibrium the pressure jump dP across the interface is proportional to
/// 1/R (dP = sigma/R), so sigma = dP*R is constant across radii and dP decreases
/// with R. Pressure is the Shan-Chen equation of state.

#include <array>
#include <cmath>
#include <cstdint>

#include "cyberfluids/physics/shan_chen_eos.hpp"
#include "cyberfluids/solver/shan_chen_2d.hpp"
#include "testing.hpp"

namespace {
double avgDensity(cyberfluids::solver::ShanChen2D<>& sc, std::int64_t x0, std::int64_t y0,
                  std::int64_t hw) {
    double s = 0;
    std::int64_t n = 0;
    for (std::int64_t x = x0 - hw; x <= x0 + hw; ++x)
        for (std::int64_t y = y0 - hw; y <= y0 + hw; ++y) {
            s += sc.density(x, y);
            ++n;
        }
    return s / n;
}

// sigma-estimate = dP*R for a droplet of radius R.
double sigmaEstimate(double R, double& dPout) {
    const double G = -5.0, rho0 = 1.0, cs2 = 1.0 / 3.0;
    const std::int64_t N = 80;
    cyberfluids::solver::ShanChen2D<> sc(N, N, 1.0, {G, rho0});
    sc.initDroplet(1.87, 0.12, R);
    sc.run(6000);
    const double rl = avgDensity(sc, N / 2, N / 2, 2);
    const double rv = avgDensity(sc, 3, 3, 2);
    const double dP = cyberfluids::shanChenPressure(rl, G, rho0, cs2) -
                      cyberfluids::shanChenPressure(rv, G, rho0, cs2);
    dPout = dP;
    return dP * R;
}
}  // namespace

int main() {
    const double radii[3] = {12.0, 18.0, 24.0};
    double dP[3], sigma[3];
    for (int i = 0; i < 3; ++i) sigma[i] = sigmaEstimate(radii[i], dP[i]);

    std::printf("shan_chen_laplace: R=%.0f/%.0f/%.0f  dP=%.5f/%.5f/%.5f  sigma=%.4f/%.4f/%.4f\n",
                radii[0], radii[1], radii[2], dP[0], dP[1], dP[2], sigma[0], sigma[1], sigma[2]);

    // Positive pressure jump, decreasing with R (Laplace signature).
    for (int i = 0; i < 3; ++i) CF_CHECK(dP[i] > 0.0);
    CF_CHECK(dP[0] > dP[1] && dP[1] > dP[2]);

    // sigma = dP*R constant across radii (within 5% of the mean).
    const double mean = (sigma[0] + sigma[1] + sigma[2]) / 3.0;
    CF_CHECK(mean > 0.0);
    for (int i = 0; i < 3; ++i) CF_CHECK(std::fabs(sigma[i] - mean) < 0.05 * mean);

    if (cftest::failures == 0) std::printf("shan_chen_laplace: all checks passed\n");
    return cftest::failures;
}
