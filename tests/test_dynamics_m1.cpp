/// Unit tests for the M1 collision models: TRT, MRT, regularized, forced.
/// Key checks: TRT and MRT reduce to BGK in the appropriate limit; every model
/// conserves mass; equilibrium is a collision fixed point; regularization
/// preserves the non-equilibrium stress; forcing injects exactly F of momentum.

#include <array>
#include <cmath>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/dynamics/forced.hpp"
#include "cyberfluids/dynamics/mrt.hpp"
#include "cyberfluids/dynamics/regularized.hpp"
#include "cyberfluids/dynamics/trt.hpp"
#include "cyberfluids/solver/lid_driven_cavity.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::D2Q9;
using Lattice = cyberfluids::BlockLattice2D<double, D2Q9>;
using BGK = cyberfluids::BGKdynamics<double, D2Q9>;

namespace {

std::array<double, 9> sampleState() {
    std::array<double, 9> f{};
    for (int i = 0; i < 9; ++i) f[i] = 0.1 + 0.01 * i;  // arbitrary, positive
    return f;
}

void setCell(Lattice& lat, const std::array<double, 9>& f) {
    auto cell = lat.get(0, 0);
    for (int i = 0; i < 9; ++i) cell[i] = f[i];
}

double mass(Lattice& lat) {
    double m = 0;
    auto cell = lat.get(0, 0);
    for (int i = 0; i < 9; ++i) m += cell[i];
    return m;
}

std::array<double, 2> momentum(Lattice& lat) {
    std::array<double, 2> j{};
    auto cell = lat.get(0, 0);
    for (int i = 0; i < 9; ++i)
        for (int a = 0; a < 2; ++a) j[a] += cell[i] * D2Q9::c[i][a];
    return j;
}

}  // namespace

int main() {
    const double omega = 1.0 / 0.6;
    const auto f0 = sampleState();

    // --- TRT reduces to BGK when omega_minus == omega_plus ---
    {
        const double tauMinusHalf = 1.0 / omega - 0.5;
        const double magic = tauMinusHalf * tauMinusHalf;  // forces omega_minus == omega
        cyberfluids::TRTdynamics<double, D2Q9> trt(omega, magic);
        CF_CHECK_CLOSE(trt.getOmegaMinus(), omega, 1e-12);

        Lattice a(1, 1), b(1, 1);
        setCell(a, f0);
        setCell(b, f0);
        auto ca = a.get(0, 0);
        auto cb = b.get(0, 0);
        BGK{omega}.collide(ca);
        trt.collide(cb);
        for (int i = 0; i < 9; ++i) CF_CHECK_CLOSE(ca[i], cb[i], 1e-12);
    }

    // --- MRT reduces to BGK when all free rates == omega ---
    {
        cyberfluids::MRTdynamics<double, D2Q9> mrt(omega, omega, omega, omega);
        Lattice a(1, 1), b(1, 1);
        setCell(a, f0);
        setCell(b, f0);
        auto ca = a.get(0, 0);
        auto cb = b.get(0, 0);
        BGK{omega}.collide(ca);
        mrt.collide(cb);
        for (int i = 0; i < 9; ++i) CF_CHECK_CLOSE(ca[i], cb[i], 1e-11);
    }

    // --- Conservation + equilibrium fixed point for TRT, MRT, Regularized ---
    {
        cyberfluids::TRTdynamics<double, D2Q9> trt(omega);
        cyberfluids::MRTdynamics<double, D2Q9> mrt(omega);
        cyberfluids::RegularizedBGKdynamics<double, D2Q9> reg(omega);
        cyberfluids::Dynamics<double, D2Q9>* models[] = {&trt, &mrt, &reg};

        for (auto* dyn : models) {
            Lattice lat(1, 1);
            setCell(lat, f0);
            const double m0 = mass(lat);
            auto cell = lat.get(0, 0);
            dyn->collide(cell);
            CF_CHECK_CLOSE(mass(lat), m0, 1e-12);  // mass conserved

            // Equilibrium is a fixed point.
            const double rho = 1.1;
            const std::array<double, 2> u{0.03, -0.02};
            const double uSqr = u[0] * u[0] + u[1] * u[1];
            Lattice eqLat(1, 1);
            auto eqCell = eqLat.get(0, 0);
            for (int i = 0; i < 9; ++i) eqCell[i] = BGK::equilibrium(i, rho, u, uSqr);
            std::array<double, 9> before{};
            for (int i = 0; i < 9; ++i) before[i] = eqCell[i];
            dyn->collide(eqCell);
            for (int i = 0; i < 9; ++i) CF_CHECK_CLOSE(eqCell[i], before[i], 1e-11);
        }
    }

    // --- Regularized: the projected non-equilibrium reproduces Pi (scaled by 1-omega) ---
    {
        const double om = 0.5;
        cyberfluids::RegularizedBGKdynamics<double, D2Q9> reg(om);
        Lattice lat(1, 1);
        setCell(lat, f0);
        auto cell = lat.get(0, 0);
        double rho = 0;
        std::array<double, 2> j{};
        for (int i = 0; i < 9; ++i) {
            rho += cell[i];
            for (int a = 0; a < 2; ++a) j[a] += cell[i] * D2Q9::c[i][a];
        }
        std::array<double, 2> u{j[0] / rho, j[1] / rho};
        const double uSqr = u[0] * u[0] + u[1] * u[1];
        double Pi0[2][2] = {{0, 0}, {0, 0}};
        for (int i = 0; i < 9; ++i) {
            const double fneq = cell[i] - BGK::equilibrium(i, rho, u, uSqr);
            for (int a = 0; a < 2; ++a)
                for (int b = 0; b < 2; ++b) Pi0[a][b] += D2Q9::c[i][a] * D2Q9::c[i][b] * fneq;
        }
        reg.collide(cell);
        double Pi1[2][2] = {{0, 0}, {0, 0}};
        for (int i = 0; i < 9; ++i) {
            const double fneq = cell[i] - BGK::equilibrium(i, rho, u, uSqr);
            for (int a = 0; a < 2; ++a)
                for (int b = 0; b < 2; ++b) Pi1[a][b] += D2Q9::c[i][a] * D2Q9::c[i][b] * fneq;
        }
        for (int a = 0; a < 2; ++a)
            for (int b = 0; b < 2; ++b) CF_CHECK_CLOSE(Pi1[a][b], (1 - om) * Pi0[a][b], 1e-12);
    }

    // --- Forced: F=0 == BGK; mass conserved; momentum increases by exactly F ---
    {
        const std::array<double, 2> F{1e-4, -5e-5};
        cyberfluids::ForcedBGKdynamics<double, D2Q9> forced(omega, F);

        Lattice zero(1, 1), bgkLat(1, 1);
        setCell(zero, f0);
        setCell(bgkLat, f0);
        cyberfluids::ForcedBGKdynamics<double, D2Q9> unforced(omega, {0.0, 0.0});
        auto cz = zero.get(0, 0);
        auto cbgk = bgkLat.get(0, 0);
        unforced.collide(cz);
        BGK{omega}.collide(cbgk);
        for (int i = 0; i < 9; ++i) CF_CHECK_CLOSE(cz[i], cbgk[i], 1e-12);

        Lattice lat(1, 1);
        setCell(lat, f0);
        const double m0 = mass(lat);
        const auto j0 = momentum(lat);
        auto cf = lat.get(0, 0);
        forced.collide(cf);
        CF_CHECK_CLOSE(mass(lat), m0, 1e-12);
        const auto j1 = momentum(lat);
        CF_CHECK_CLOSE(j1[0] - j0[0], F[0], 1e-12);
        CF_CHECK_CLOSE(j1[1] - j0[1], F[1], 1e-12);
    }

    // --- Integration: TRT / MRT / Regularized run through the cavity solver ---
    // (same solver, different collision model — no core changes). Each must stay
    // stable and reproduce the lid-driven top>bottom velocity asymmetry.
    {
        namespace sol = cyberfluids::solver;
        const std::int64_t n = 32;
        const double om = 1.0, U = 0.1;

        sol::LidDrivenCavity2D<cyberfluids::backend::Default, double,
                               cyberfluids::TRTdynamics<double, D2Q9>> trtCav(n, n, om, U);
        sol::LidDrivenCavity2D<cyberfluids::backend::Default, double,
                               cyberfluids::MRTdynamics<double, D2Q9>> mrtCav(n, n, om, U);
        sol::LidDrivenCavity2D<cyberfluids::backend::Default, double,
                               cyberfluids::RegularizedBGKdynamics<double, D2Q9>> regCav(n, n, om, U);

        trtCav.run(2000);
        mrtCav.run(2000);
        regCav.run(2000);

        auto stableAndDriven = [n](auto& cav) {
            bool finite = true;
            double umax = 0.0;
            for (std::int64_t x = 0; x < n; ++x)
                for (std::int64_t y = 0; y < n; ++y) {
                    const auto u = cav.velocity(x, y);
                    if (!std::isfinite(u[0]) || !std::isfinite(u[1])) finite = false;
                    umax = std::max(umax, std::sqrt(u[0] * u[0] + u[1] * u[1]));
                }
            const double uxTop = cav.velocity(n / 2, n - 2)[0];
            const double uxBot = cav.velocity(n / 2, 1)[0];
            CF_CHECK(finite);
            CF_CHECK(umax < 0.3);
            CF_CHECK(uxTop > 0.0 && uxTop > uxBot);
        };
        stableAndDriven(trtCav);
        stableAndDriven(mrtCav);
        stableAndDriven(regCav);
    }

    if (cftest::failures == 0) std::printf("dynamics_m1: all checks passed\n");
    return cftest::failures;
}
