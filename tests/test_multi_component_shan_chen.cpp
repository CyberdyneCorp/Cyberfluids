/// Multi-component (two-species) Shan-Chen validation: the inter-species force is
/// computed exactly (including a periodic-wrap neighbour) with psi = rho, a
/// uniform "other" density gives zero force, and — the load-bearing physics test
/// — two initially mixed species spontaneously de-mix above the critical coupling
/// (and stay mixed below it), each species' mass conserved.

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/dynamics/forced.hpp"
#include "cyberfluids/solver/multi_component_shan_chen_2d.hpp"
#include "cyberfluids/timestep/shan_chen.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::ForcedD2Q9;
using FluidL = cyberfluids::BlockLattice2D<double, ForcedD2Q9>;
using FBGK = cyberfluids::ExternalForceBGKdynamics<double, ForcedD2Q9>;

int main() {
    const double G = 3.0;
    const double w1 = 1.0 / 9.0;  // SC weight for an axial link = lattice weight t_i

    // 1. Exact cross force: species A uniform, species B uniform except one
    //    elevated cell; only A's neighbour of that cell feels a force. psi = rho.
    {
        FluidL a(4, 4);
        a.attributeDynamics(a.getBoundingBox(), std::make_shared<FBGK>(1.0));
        std::vector<double> rhoA(16, 1.0), rhoB(16, 1.0);
        rhoB[a.index(2, 2)] = 1.5;  // B elevated at (2,2)
        cyberfluids::applyInterComponentForce(a, rhoA.data(), rhoB.data(), G);
        // (1,2) sees B's elevated cell at its +x neighbour: dpsi = 1.5 - 1.0 = 0.5.
        const double expFx = -G * /*psiSelf=rhoA*/ 1.0 * w1 * 0.5;
        CF_CHECK_CLOSE(a.externalField()(0, a.index(1, 2)), expFx, 1e-12);
        CF_CHECK_CLOSE(a.externalField()(1, a.index(1, 2)), 0.0, 1e-12);

        // Periodic wrap: elevate B at (0,2); A cell (3,2)'s +x neighbour wraps to (0,2).
        std::vector<double> rhoBw(16, 1.0);
        rhoBw[a.index(0, 2)] = 1.5;
        cyberfluids::applyInterComponentForce(a, rhoA.data(), rhoBw.data(), G);
        CF_CHECK_CLOSE(a.externalField()(0, a.index(3, 2)), expFx, 1e-12);
    }

    // 2. Uniform "other" density -> zero force (sum_i w_i c_i = 0).
    {
        FluidL a(8, 8);
        a.attributeDynamics(a.getBoundingBox(), std::make_shared<FBGK>(1.0));
        std::vector<double> rhoA(64, 1.3), rhoB(64, 0.9);
        cyberfluids::applyInterComponentForce(a, rhoA.data(), rhoB.data(), G);
        for (std::int64_t c = 0; c < a.ncells(); ++c) {
            CF_CHECK_CLOSE(a.externalField()(0, c), 0.0, 1e-12);
            CF_CHECK_CLOSE(a.externalField()(1, c), 0.0, 1e-12);
        }
    }

    // 3. Spontaneous de-mixing above critical G; stays mixed below; mass conserved.
    //    psi=rho, tau=1: onset at G*mean ~= 1. mean=0.7 gives a robust stable
    //    window; G=1.8 (G*mean=1.26) de-mixes, G=1.2 (G*mean=0.84) stays mixed.
    {
        using MC = cyberfluids::solver::MultiComponentShanChen2D<>;
        MC sup(64, 64, 1.0, 1.8);  // above critical -> de-mixes
        sup.initMixed(0.7, 0.035);
        const auto m0 = sup.totalMass();
        const double seg0 = sup.segregation();
        sup.run(3000);
        const double seg1 = sup.segregation();
        const auto m1 = sup.totalMass();
        std::printf("mc_shan_chen: segregation %.4f -> %.4f\n", seg0, seg1);
        CF_CHECK(seg1 > 0.5);           // strongly separated
        CF_CHECK(seg1 > seg0 + 0.3);    // grew from the mixed seed
        CF_CHECK_CLOSE(m1[0], m0[0], 1e-6 * m0[0]);  // species A mass conserved
        CF_CHECK_CLOSE(m1[1], m0[1], 1e-6 * m0[1]);  // species B mass conserved

        MC sub(64, 64, 1.0, 1.2);  // below critical -> stays mixed
        sub.initMixed(0.7, 0.035);
        sub.run(3000);
        const double segSub = sub.segregation();
        std::printf("mc_shan_chen: sub-critical segregation %.4f\n", segSub);
        CF_CHECK(segSub < 0.1);  // no de-mixing
    }

    if (cftest::failures == 0) std::printf("mc_shan_chen: all checks passed\n");
    return cftest::failures;
}
