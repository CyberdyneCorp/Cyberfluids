/// Verifies the buoyancy coupling: the force map F_y = rho*g*beta*(T - T_ref)
/// (F_x = 0) is written exactly, a geometry mismatch throws, and a sub-box
/// fluid (some null-dynamics cells) does not crash.

#include <array>
#include <cstdint>
#include <memory>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/dynamics/advection_diffusion.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/dynamics/forced.hpp"
#include "cyberfluids/timestep/buoyancy.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::AdvectedD2Q5;
using cyberfluids::descriptors::ForcedD2Q9;
using FluidL = cyberfluids::BlockLattice2D<double, ForcedD2Q9>;
using TempL = cyberfluids::BlockLattice2D<double, AdvectedD2Q5>;
using FBGK = cyberfluids::ExternalForceBGKdynamics<double, ForcedD2Q9>;
using AD = cyberfluids::AdvectionDiffusionBGKdynamics<double, AdvectedD2Q5>;

namespace {
void initTemp(TempL& t, double phi) {
    for (std::int64_t c = 0; c < t.ncells(); ++c)
        for (int i = 0; i < 5; ++i) t.populations()(i, c) = AD::equilibrium(i, phi, {0.0, 0.0});
}
void initFluidRest(FluidL& f) {
    for (std::int64_t c = 0; c < f.ncells(); ++c)
        for (int i = 0; i < 9; ++i)
            f.populations()(i, c) =
                cyberfluids::BGKdynamics<double, ForcedD2Q9>::equilibrium(i, 1.0, {0.0, 0.0}, 0.0);
}
}  // namespace

int main() {
    const std::int64_t n = 8;
    const double g = 0.001, beta = 1.0, Tref = 0.5, Tval = 0.8;
    cyberfluids::BoussinesqParameters<double> p;
    p.gravity = g;
    p.thermalExpansion = beta;
    p.referenceTemperature = Tref;
    p.gravityAxis = 1;

    FluidL fluid(n, n);
    fluid.attributeDynamics(fluid.getBoundingBox(), std::make_shared<FBGK>(1.0));
    initFluidRest(fluid);
    TempL temp(n, n);
    temp.attributeDynamics(temp.getBoundingBox(), std::make_shared<AD>(1.0));
    initTemp(temp, Tval);

    cyberfluids::applyBuoyancy(fluid, temp, p);
    const double expected = 1.0 * g * beta * (Tval - Tref);
    for (std::int64_t c = 0; c < fluid.ncells(); ++c) {
        CF_CHECK_CLOSE(fluid.externalField()(1, c), expected, 1e-12);  // F_y
        CF_CHECK(fluid.externalField()(0, c) == 0.0);                  // F_x
    }

    // Geometry mismatch rejected.
    {
        TempL bad(n, n + 1);
        bad.attributeDynamics(bad.getBoundingBox(), std::make_shared<AD>(1.0));
        bool threw = false;
        try {
            cyberfluids::applyBuoyancy(fluid, bad, p);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CF_CHECK(threw);
    }

    // Sub-box fluid (unassigned cells) must not crash; they use rho0.
    {
        FluidL fsub(n, n);
        cyberfluids::Box<2> inner;
        inner.lo = {1, 1};
        inner.hi = {n - 2, n - 2};
        fsub.attributeDynamics(inner, std::make_shared<FBGK>(1.0));
        initFluidRest(fsub);
        cyberfluids::applyBuoyancy(fsub, temp, p);  // must not crash
        CF_CHECK_CLOSE(fsub.externalField()(1, fsub.index(0, 0)), expected, 1e-12);  // rho0=1
    }

    if (cftest::failures == 0) std::printf("buoyancy: all checks passed\n");
    return cftest::failures;
}
