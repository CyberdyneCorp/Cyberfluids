/// Validates partial bounce-back (porous media) dynamics:
///   T1  ns = 0 collide is bit-exact plain BGK (the convex blend degenerates).
///   T2  ns = 1 collide is bit-exact the population swap f_i <- f_opposite(i)
///       (a no-slip node), and conserves mass while reversing momentum.
///   T3  in a forced periodic box the steady through-flow decreases strictly and
///       monotonically as the uniform solid fraction ns rises.
///   T4  the through-flow obeys the analytic Darcy law U = (1-ns) Fx / (2 ns):
///       linear in Fx at fixed ns, with apparent permeability falling with ns.
/// T1/T2 are exact algebraic identities; T3/T4 use the self-contained analytic
/// oracle derived from the collision fixed point (no external reference data).

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/dynamics/porous.hpp"
#include "cyberfluids/solver/porous_flow_2d.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::D2Q9;
using cyberfluids::descriptors::PorousD2Q9;
using PorousDyn = cyberfluids::PorousForcedBGKdynamics<double, PorousD2Q9>;
using BgkDyn = cyberfluids::BGKdynamics<double, D2Q9>;

namespace {

// A fixed, non-equilibrium population set for the exact single-cell identities.
std::array<double, 9> seedPops() {
    return {0.11, 0.05, 0.07, 0.03, 0.09, 0.02, 0.06, 0.04, 0.08};
}

}  // namespace

int main() {
    const double omegas[] = {0.5, 1.0, 1.8};

    // ---- T1: ns = 0 collide == plain BGK, bit-exact. -----------------------
    for (double omega : omegas) {
        const auto f = seedPops();

        cyberfluids::BlockLattice2D<double, PorousD2Q9> plat(1, 1);
        auto pdyn = std::make_shared<PorousDyn>(omega, std::array<double, 2>{0.0, 0.0});
        plat.attributeDynamics(plat.getBoundingBox(), pdyn);
        plat.externalField()(PorousDyn::nsOffset, 0) = 0.0;  // pure fluid
        for (int i = 0; i < 9; ++i) plat.populations()(i, 0) = f[i];
        auto pcell = plat.get(0, 0);
        pdyn->collide(pcell);

        cyberfluids::BlockLattice2D<double, D2Q9> blat(1, 1);
        auto bdyn = std::make_shared<BgkDyn>(omega);
        blat.attributeDynamics(blat.getBoundingBox(), bdyn);
        for (int i = 0; i < 9; ++i) blat.populations()(i, 0) = f[i];
        auto bcell = blat.get(0, 0);
        bdyn->collide(bcell);

        for (int i = 0; i < 9; ++i)
            CF_CHECK(plat.populations()(i, 0) == blat.populations()(i, 0));
    }

    // ---- T2: ns = 1 collide == population swap f_i <- f_opposite(i). --------
    for (double omega : omegas) {
        const auto f = seedPops();

        cyberfluids::BlockLattice2D<double, PorousD2Q9> plat(1, 1);
        auto pdyn = std::make_shared<PorousDyn>(omega, std::array<double, 2>{1e-4, 0.0});
        plat.attributeDynamics(plat.getBoundingBox(), pdyn);
        plat.externalField()(PorousDyn::nsOffset, 0) = 1.0;  // full solid
        for (int i = 0; i < 9; ++i) plat.populations()(i, 0) = f[i];
        auto pcell = plat.get(0, 0);
        pdyn->collide(pcell);

        // The exact per-direction swap fully pins the output; mass-conservation
        // and momentum-reversal are then algebraic identities of the opposite
        // permutation, so asserting them would add no independent coverage.
        for (int i = 0; i < 9; ++i)
            CF_CHECK(plat.populations()(i, 0) == f[D2Q9::opposite[i]]);  // exact swap
    }

    // ---- T3: monotonic through-flow decrease with ns. ----------------------
    {
        const double omega = 1.0, Fx = 1e-5;
        const double nsVals[] = {0.02, 0.05, 0.1, 0.2, 0.4};
        double prev = 1e30;
        for (double ns : nsVals) {
            cyberfluids::solver::PorousBox2D<> box(32, 32, omega, Fx);
            box.setUniformSolidFraction(ns);
            box.run(20000);
            const double u = box.meanFluxVelocityX();
            CF_CHECK(u > 0.0);
            CF_CHECK(u < prev);  // strictly decreasing
            prev = u;
        }
    }

    // ---- T4: Darcy law — flux matches the exact closed-form oracle. ---------
    // The fixed point j = (1-ns)Fx/(2ns) is exact (the first moment closes
    // affinely in Fx) and fully converged after 20000 steps, so the residual is
    // roundoff-limited. A NEAR-EXACT tolerance is what makes this test actually
    // guard the (1-ns) force attenuation and the Guo (1-omega/2) prefactor —
    // matching each (ns, Fx) to the analytic value (which is ∝ Fx) also pins
    // Darcy linearity, so no separate (vacuously-true) linearity check is needed.
    {
        const double omega = 1.0;
        const double nu = cyberfluids::solver::PorousBox2D<>::kinematicViscosity(omega);
        const double nsVals[] = {0.05, 0.1, 0.2};
        const double forces[] = {2.5e-6, 5e-6, 1e-5, 2e-5};
        double prevK = 1e30;
        for (double ns : nsVals) {
            double k = 0.0;
            for (double Fx : forces) {
                cyberfluids::solver::PorousBox2D<> box(32, 32, omega, Fx);
                box.setUniformSolidFraction(ns);
                box.run(20000);
                const double u = box.meanFluxVelocityX();
                const double analytic = box.analyticFluxVelocityX(ns);
                CF_CHECK_CLOSE(u, analytic, 1e-9 * analytic);  // exact oracle
                k = nu * u / Fx;  // apparent permeability (Fx-independent)
            }
            CF_CHECK(k < prevK);  // permeability k = nu(1-ns)/(2ns) falls with ns
            prevK = k;
        }
    }

    // ---- T5: spatial ns exercises streaming + per-cell reads; mass conserved.
    // T3/T4 keep the field spatially uniform, so streaming is a no-op there. A
    // graded ns makes streaming move populations between cells of differing ns.
    // The blend and Guo source are each mass-neutral per cell and periodic
    // streaming is a permutation, so total mass must be conserved to roundoff;
    // a blend-weight or source-mass bug (or a dropped-population streaming bug)
    // would break it.
    {
        const double omega = 1.0, Fx = 1e-5;
        cyberfluids::solver::PorousBox2D<> box(32, 32, omega, Fx);
        box.setSolidFraction([](std::int64_t x, std::int64_t /*y*/) {
            return 0.05 + 0.4 * static_cast<double>(x) / 32.0;  // graded across x
        });
        const double m0 = 32.0 * 32.0;  // rho = 1 in every cell initially
        box.run(3000);
        auto& pop = box.lattice().populations();
        double mass = 0.0;
        for (std::int64_t c = 0; c < box.nx() * box.ny(); ++c)
            for (int i = 0; i < 9; ++i) mass += pop(i, c);
        CF_CHECK_CLOSE(mass, m0, 1e-8);
    }

    if (cftest::failures == 0) std::printf("porous: all checks passed\n");
    return cftest::failures;
}
