/// Convection-onset validation: sub-critical (Ra < Ra_c ~= 1708) a seeded
/// perturbation stays quiescent (no convection, KE ~ 0); super-critical
/// (Ra > Ra_c) it develops finite-amplitude convection (KE many orders of
/// magnitude larger). Same box, gravity and perturbation — only the relaxation
/// rates differ (the dimensionless mapping fixes g*beta*dT independent of Ra).

#include <cstdint>

#include "cyberfluids/solver/rayleigh_benard.hpp"
#include "testing.hpp"

namespace {
// Steady-state mean kinetic energy for a given Rayleigh number.
double steadyKE(double Ra) {
    const std::int64_t nx = 40, ny = 20;  // aspect 2 -> perturbation kH ~= pi (near-critical mode)
    const double Pr = 1.0, Uf = 0.1;
    auto rb = cyberfluids::solver::RayleighBenard2D<>::fromDimensionless(nx, ny, Ra, Pr, Uf);
    rb.perturbTemperature(1e-3);
    rb.run(20000);
    return rb.avgKineticEnergy();
}
}  // namespace

int main() {
    const double keSub = steadyKE(700.0);      // below Ra_c
    const double keSuper = steadyKE(10000.0);  // above Ra_c
    std::printf("rayleigh_benard: KE sub(Ra=700)=%.3e  super(Ra=10000)=%.3e  ratio=%.1e\n",
                keSub, keSuper, keSuper / keSub);

    CF_CHECK(keSub < 1e-7);              // sub-critical: no convection
    CF_CHECK(keSuper > 1e-6);            // super-critical: convection developed
    CF_CHECK(keSuper > 1e4 * keSub);     // unambiguous onset contrast

    if (cftest::failures == 0) std::printf("rayleigh_benard: all checks passed\n");
    return cftest::failures;
}
