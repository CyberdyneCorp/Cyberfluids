/// Validates fluid -> AD coupling: fluid velocity is written into the AD
/// lattice's external velocity field, a geometry mismatch is rejected, and an
/// end-to-end fluid->couple->AD step advects a scalar by the fluid velocity.

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/dynamics/advection_diffusion.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/timestep/coupling.hpp"
#include "cyberfluids/timestep/evolve.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::AdvectedD2Q5;
using cyberfluids::descriptors::D2Q9;
using FluidD = D2Q9;
using ADd = AdvectedD2Q5;
using FluidLattice = cyberfluids::BlockLattice2D<double, FluidD>;
using ADLattice = cyberfluids::BlockLattice2D<double, ADd>;
using BGK = cyberfluids::BGKdynamics<double, FluidD>;
using AD = cyberfluids::AdvectionDiffusionBGKdynamics<double, ADd>;

namespace {
void initFluidAtVelocity(FluidLattice& fluid, std::array<double, 2> u) {
    const double uSqr = u[0] * u[0] + u[1] * u[1];
    for (std::int64_t c = 0; c < fluid.ncells(); ++c)
        for (int i = 0; i < 9; ++i)
            fluid.populations()(i, c) = BGK::equilibrium(i, 1.0, u, uSqr);
}
}  // namespace

int main() {
    const std::int64_t n = 8;
    const std::array<double, 2> u0{0.05, 0.02};

    FluidLattice fluid(n, n);
    fluid.attributeDynamics(fluid.getBoundingBox(), std::make_shared<BGK>(1.0));
    initFluidAtVelocity(fluid, u0);

    ADLattice ad(n, n);
    ad.attributeDynamics(ad.getBoundingBox(), std::make_shared<AD>(1.0));

    // Couple: fluid velocity lands in the AD external velocity slots.
    cyberfluids::copyVelocityToExternal(fluid, ad);
    for (std::int64_t c = 0; c < ad.ncells(); ++c) {
        CF_CHECK_CLOSE(ad.externalField()(0, c), u0[0], 1e-12);
        CF_CHECK_CLOSE(ad.externalField()(1, c), u0[1], 1e-12);
    }

    // Geometry mismatch is rejected.
    {
        ADLattice adBad(n, n + 1);
        bool threw = false;
        try {
            cyberfluids::copyVelocityToExternal(fluid, adBad);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CF_CHECK(threw);
    }

    // End-to-end: seed a scalar blob on a wide channel, couple, advance AD; the
    // centroid moves by u*t. Wide (48x1) so diffusion never reaches the periodic
    // boundary (which would corrupt the wrapped-centroid metric).
    {
        const std::int64_t nx = 48;
        FluidLattice fluidW(nx, 1);
        fluidW.attributeDynamics(fluidW.getBoundingBox(), std::make_shared<BGK>(1.0));
        initFluidAtVelocity(fluidW, u0);

        ADLattice adW(nx, 1);
        adW.attributeDynamics(adW.getBoundingBox(), std::make_shared<AD>(1.0));
        const double x0 = 8.0, sigma = 3.0;
        for (std::int64_t x = 0; x < nx; ++x) {
            const std::int64_t c = adW.index(x, 0);
            const double phi = 0.1 + std::exp(-((x - x0) * (x - x0)) / (2.0 * sigma * sigma));
            for (int i = 0; i < 5; ++i) adW.populations()(i, c) = AD::equilibrium(i, phi, u0);
        }
        cyberfluids::copyVelocityToExternal(fluidW, adW);
        cyberfluids::PopulationField<double, ADd> scratch(adW.ncells());
        const std::int64_t T = 100;
        for (std::int64_t s = 0; s < T; ++s) cyberfluids::collideAndStream(adW, scratch);

        double num = 0.0, den = 0.0;
        for (std::int64_t x = 0; x < nx; ++x) {
            const double w = [&] {
                auto cell = adW.get(x, 0);
                double phi = 0;
                for (int i = 0; i < 5; ++i) phi += cell[i];
                return phi - 0.1;
            }();
            num += x * w;
            den += w;
        }
        const double centroid = num / den;
        const double expected = x0 + u0[0] * T;  // 8 + 0.05*100 = 13
        std::printf("ad_coupling: centroid=%.3f expected=%.3f\n", centroid, expected);
        CF_CHECK(std::fabs(centroid - expected) < 1.5);
    }

    if (cftest::failures == 0) std::printf("ad_coupling: all checks passed\n");
    return cftest::failures;
}
