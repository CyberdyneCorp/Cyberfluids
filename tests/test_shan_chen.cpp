/// Shan-Chen core validation: the non-local force is computed exactly (including
/// a periodic-wrap neighbour), a uniform density gives zero force, and — the
/// load-bearing physics test — the fluid spontaneously separates above the
/// critical G (and stays homogeneous below it), conserving mass.

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/dynamics/forced.hpp"
#include "cyberfluids/physics/shan_chen_eos.hpp"
#include "cyberfluids/solver/shan_chen_2d.hpp"
#include "cyberfluids/timestep/shan_chen.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::ForcedD2Q9;
using FluidL = cyberfluids::BlockLattice2D<double, ForcedD2Q9>;
using FBGK = cyberfluids::ExternalForceBGKdynamics<double, ForcedD2Q9>;

int main() {
    const double rho0 = 1.0, G = -6.0;
    const double w1 = 1.0 / 9.0;  // SC weight for an axial link = lattice weight t_i
    const double dpsi = cyberfluids::shanChenPsi(1.5, rho0) - cyberfluids::shanChenPsi(1.0, rho0);

    // 1. Exact force: uniform rho=1 with one elevated cell; only the +x neighbour differs.
    {
        FluidL f(4, 4);
        f.attributeDynamics(f.getBoundingBox(), std::make_shared<FBGK>(1.0));
        cyberfluids::ShanChenParameters<double> p{G, rho0};

        std::vector<double> rho(16, 1.0);
        rho[f.index(2, 2)] = 1.5;
        cyberfluids::applyShanChenForce(f, rho.data(), p);
        const double expFx = -G * cyberfluids::shanChenPsi(1.0, rho0) * w1 * dpsi;
        CF_CHECK_CLOSE(f.externalField()(0, f.index(1, 2)), expFx, 1e-12);  // (1,2) sees (2,2) at +x
        CF_CHECK_CLOSE(f.externalField()(1, f.index(1, 2)), 0.0, 1e-12);

        // Periodic wrap: elevate (0,2); cell (3,2)'s +x neighbour wraps to (0,2).
        std::vector<double> rhoW(16, 1.0);
        rhoW[f.index(0, 2)] = 1.5;
        cyberfluids::applyShanChenForce(f, rhoW.data(), p);
        CF_CHECK_CLOSE(f.externalField()(0, f.index(3, 2)), expFx, 1e-12);
    }

    // 2. Uniform density -> zero force (sum_i w_i c_i = 0).
    {
        FluidL f(8, 8);
        f.attributeDynamics(f.getBoundingBox(), std::make_shared<FBGK>(1.0));
        std::vector<double> rho(64, 1.3);
        cyberfluids::applyShanChenForce(f, rho.data(), {G, rho0});
        for (std::int64_t c = 0; c < f.ncells(); ++c) {
            CF_CHECK_CLOSE(f.externalField()(0, c), 0.0, 1e-12);
            CF_CHECK_CLOSE(f.externalField()(1, c), 0.0, 1e-12);
        }
    }

    // 3. Spontaneous separation above critical G; homogeneous below; mass conserved.
    {
        using SC = cyberfluids::solver::ShanChen2D<>;
        SC sup(64, 64, 1.0, {-5.0, rho0});  // |G|=5 above critical (~4.2) -> separates
        sup.initDensity(1.0, 0.01);
        const double m0 = sup.totalMass();
        sup.run(4000);
        const auto mm = sup.minMaxDensity();
        std::printf("shan_chen: super-critical density [%.4f, %.4f]\n", mm[0], mm[1]);
        CF_CHECK(std::isfinite(mm[0]) && std::isfinite(mm[1]));
        CF_CHECK(mm[1] - mm[0] > 1.0);                        // liquid/vapour gap
        CF_CHECK_CLOSE(sup.totalMass(), m0, 1e-6 * m0);       // mass conserved

        SC sub(64, 64, 1.0, {-2.0, rho0});  // |G|=2 < 4 -> stays homogeneous
        sub.initDensity(1.0, 0.01);
        sub.run(4000);
        const auto mm2 = sub.minMaxDensity();
        std::printf("shan_chen: sub-critical density [%.4f, %.4f]\n", mm2[0], mm2[1]);
        CF_CHECK(mm2[1] - mm2[0] < 0.05);                     // no separation
    }

    if (cftest::failures == 0) std::printf("shan_chen: all checks passed\n");
    return cftest::failures;
}
