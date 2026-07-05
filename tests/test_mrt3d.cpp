/// Validates the D3Q19 MRT collision operator:
///   T1  MRTdynamics3D with every free rate == omega is bit-exact BGK (the
///       moment-space operator M^-1 (I - omega)(M f - M feq) collapses to
///       f - omega(f - feq)), for a random non-equilibrium cell.
///   T2  ForcedMRTdynamics3D with every free rate == omega is bit-exact
///       ForcedBGKdynamics (same reduction with the Guo source).
///   T3  a forced D3Q19 Poiseuille channel driven by MRT recovers the analytic
///       parabola to <1% (and matches the forced-BGK channel), confirming the
///       viscosity nu = cs2(1/omega - 1/2) is set by the shear moments.

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/dynamics/forced.hpp"
#include "cyberfluids/dynamics/mrt3d.hpp"
#include "cyberfluids/solver/poiseuille_channel3d.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::D3Q19;
using cyberfluids::descriptors::ForcedD3Q19;

namespace {

// A fixed, non-equilibrium population set (near a light-density resting state).
std::array<double, 19> seedPops() {
    std::array<double, 19> f{};
    for (int i = 0; i < 19; ++i) f[i] = D3Q19::t[i] * (1.0 + 0.03 * (i - 9) + 0.001 * i * i);
    return f;
}

}  // namespace

int main() {
    const double omegas[] = {0.6, 1.0, 1.7};

    // ---- T1: MRT (all rates == omega) == BGK, bit-exact. -------------------
    for (double omega : omegas) {
        const auto f = seedPops();

        cyberfluids::BlockLattice3D<double, D3Q19> ml(1, 1, 1);
        // Every free rate == omega -> pure BGK relaxation in moment space.
        auto mdyn = std::make_shared<cyberfluids::MRTdynamics3D<double, D3Q19>>(
            omega, omega, omega, omega, omega, omega);
        ml.attributeDynamics(ml.getBoundingBox(), mdyn);
        for (int i = 0; i < 19; ++i) ml.populations()(i, 0) = f[i];
        auto mcell = ml.get(0, 0, 0);
        mdyn->collide(mcell);

        cyberfluids::BlockLattice3D<double, D3Q19> bl(1, 1, 1);
        auto bdyn = std::make_shared<cyberfluids::BGKdynamics<double, D3Q19>>(omega);
        bl.attributeDynamics(bl.getBoundingBox(), bdyn);
        for (int i = 0; i < 19; ++i) bl.populations()(i, 0) = f[i];
        auto bcell = bl.get(0, 0, 0);
        bdyn->collide(bcell);

        for (int i = 0; i < 19; ++i)
            CF_CHECK_CLOSE(ml.populations()(i, 0), bl.populations()(i, 0), 1e-13);
    }

    // ---- T2: forced MRT (all rates == omega) == forced BGK, bit-exact. -----
    for (double omega : omegas) {
        const auto f = seedPops();
        const std::array<double, 3> F{1e-4, -5e-5, 2e-5};

        cyberfluids::BlockLattice3D<double, ForcedD3Q19> ml(1, 1, 1);
        auto mdyn = std::make_shared<cyberfluids::ForcedMRTdynamics3D<double, ForcedD3Q19>>(
            omega, F, omega, omega, omega, omega, omega);
        ml.attributeDynamics(ml.getBoundingBox(), mdyn);
        for (int i = 0; i < 19; ++i) ml.populations()(i, 0) = f[i];
        auto mcell = ml.get(0, 0, 0);
        mdyn->collide(mcell);

        cyberfluids::BlockLattice3D<double, ForcedD3Q19> bl(1, 1, 1);
        auto bdyn =
            std::make_shared<cyberfluids::ForcedBGKdynamics<double, ForcedD3Q19>>(omega, F);
        bl.attributeDynamics(bl.getBoundingBox(), bdyn);
        for (int i = 0; i < 19; ++i) bl.populations()(i, 0) = f[i];
        auto bcell = bl.get(0, 0, 0);
        bdyn->collide(bcell);

        for (int i = 0; i < 19; ++i)
            CF_CHECK_CLOSE(ml.populations()(i, 0), bl.populations()(i, 0), 1e-13);
    }

    // ---- T3: forced-MRT D3Q19 channel recovers the analytic parabola. ------
    {
        using MrtChan = cyberfluids::solver::PoiseuilleChannel3D<
            cyberfluids::ForcedMRTdynamics3D<double, D3Q19>>;
        using BgkChan = cyberfluids::solver::PoiseuilleChannel3D<
            cyberfluids::ForcedBGKdynamics<double, D3Q19>>;
        const double omega = 1.0, Fx = 1e-5;
        const std::int64_t nx = 3, ny = 24, nz = 3;

        MrtChan mrt(nx, ny, nz, omega, Fx);
        mrt.run(20000);
        BgkChan bgk(nx, ny, nz, omega, Fx);
        bgk.run(20000);

        double uMax = 0.0;
        for (std::int64_t y = 0; y < ny; ++y) uMax = std::max(uMax, mrt.analyticUx(y, omega));

        for (std::int64_t y = 0; y < ny; ++y) {
            const double ana = mrt.analyticUx(y, omega);
            const auto um = mrt.velocity(1, y, 1);
            const auto ub = bgk.velocity(1, y, 1);
            CF_CHECK(std::fabs(um[0] - ana) / uMax < 0.01);   // MRT matches analytic
            CF_CHECK(std::fabs(um[0] - ub[0]) / uMax < 0.01);  // MRT matches BGK
            CF_CHECK(std::fabs(um[1]) < 1e-6 * uMax + 1e-12);  // no cross-flow
            CF_CHECK(std::fabs(um[2]) < 1e-6 * uMax + 1e-12);
        }
    }

    if (cftest::failures == 0) std::printf("mrt3d: all checks passed\n");
    return cftest::failures;
}
