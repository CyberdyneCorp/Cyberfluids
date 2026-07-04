/// Pure-conduction validation: with gravity off, a perturbed temperature field
/// diffuses back to the analytic linear profile between the hot/cold Dirichlet
/// plates, and the fluid never moves. Validates the anti-bounce-back walls.

#include <cmath>
#include <cstdint>

#include "cyberfluids/solver/rayleigh_benard.hpp"
#include "cyberfluids/timestep/buoyancy.hpp"
#include "testing.hpp"

int main() {
    const std::int64_t nx = 8, ny = 21;
    cyberfluids::BoussinesqParameters<double> p;
    p.gravity = 0.0;  // no buoyancy -> pure conduction
    p.thermalExpansion = 1.0;
    p.referenceTemperature = 0.5;
    p.gravityAxis = 1;

    cyberfluids::solver::RayleighBenard2D<> rb(nx, ny, /*omegaFluid=*/1.0, /*omegaTemp=*/1.0, p,
                                               /*tHot=*/1.0, /*tCold=*/0.0);
    rb.perturbTemperature(0.1);  // large perturbation; must diffuse away
    rb.run(20000);

    // Temperature matches the analytic linear profile.
    double l2 = 0.0;
    for (std::int64_t y = 0; y < ny; ++y) {
        const double d = rb.horizontallyAveragedT(y) - rb.conductionProfile(y);
        l2 += d * d;
    }
    l2 = std::sqrt(l2 / ny);
    std::printf("thermal_conduction: L2(T - linear)=%.3e  KE=%.3e\n", l2, rb.avgKineticEnergy());
    CF_CHECK(l2 < 1e-3);

    // Gravity off -> the fluid stays exactly at rest.
    CF_CHECK(rb.avgKineticEnergy() < 1e-12);

    if (cftest::failures == 0) std::printf("thermal_conduction: all checks passed\n");
    return cftest::failures;
}
