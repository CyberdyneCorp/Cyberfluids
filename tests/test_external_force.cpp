/// Validates per-cell forced dynamics: with a uniform external force it
/// reproduces the analytic Poiseuille parabola (parity with the uniform-force
/// class), and with zero external force the fluid stays at rest (proving the
/// force is read from the field, not hard-coded).

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/dynamics/forced.hpp"
#include "cyberfluids/timestep/evolve.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::ForcedD2Q9;
using D = ForcedD2Q9;
using Lattice = cyberfluids::BlockLattice2D<double, D>;
using Pop = cyberfluids::PopulationField<double, D>;

namespace {

// Run a force-driven channel (periodic in x, bounce-back y-walls) with a uniform
// per-cell external force Fx, and return the steady-state u_x(y) at column 0.
std::vector<double> runChannel(std::int64_t nx, std::int64_t ny, double omega, double Fx,
                               std::int64_t steps) {
    Lattice lat(nx, ny);
    auto dyn = std::make_shared<cyberfluids::ExternalForceBGKdynamics<double, D>>(omega);
    lat.attributeDynamics(lat.getBoundingBox(), dyn);
    for (std::int64_t c = 0; c < lat.ncells(); ++c) {
        lat.externalField()(0, c) = Fx;  // force x
        lat.externalField()(1, c) = 0.0;  // force y
        for (int i = 0; i < 9; ++i)
            lat.populations()(i, c) =
                cyberfluids::BGKdynamics<double, D>::equilibrium(i, 1.0, {0.0, 0.0}, 0.0);
    }
    Pop scratch(lat.ncells());
    for (std::int64_t s = 0; s < steps; ++s) {
        cyberfluids::collide<>(lat);
        auto& src = lat.populations();
        cyberfluids::backend::Default::forEachIndex(lat.ncells(), [&, nx, ny](std::int64_t c) {
            const std::int64_t y = c % ny, x = c / ny;
            for (int i = 0; i < 9; ++i) {
                const std::int64_t sx = ((x - D::c[i][0]) % nx + nx) % nx;
                const std::int64_t sy = y - D::c[i][1];
                if (sy >= 0 && sy < ny)
                    scratch(i, c) = src(i, sx * ny + sy);
                else
                    scratch(i, c) = src(D::opposite[i], c);
            }
        });
        std::swap(lat.populations(), scratch);
    }
    std::vector<double> ux(static_cast<std::size_t>(ny));
    for (std::int64_t y = 0; y < ny; ++y) {
        auto cell = lat.get(0, y);
        std::array<double, 2> u{};
        dyn->computeVelocity(cell, u);
        ux[static_cast<std::size_t>(y)] = u[0];
    }
    return ux;
}

}  // namespace

int main() {
    const std::int64_t nx = 8, ny = 48;
    const double omega = 1.0, Fx = 1e-5;
    const double nu = cyberfluids::ExternalForceBGKdynamics<double, D>::kinematicViscosity(omega);
    const double H = static_cast<double>(ny);

    // Uniform per-cell force -> analytic parabola.
    const auto ux = runChannel(nx, ny, omega, Fx, 30000);
    double uMax = 0.0, linf = 0.0;
    for (std::int64_t y = 0; y < ny; ++y) {
        const double Y = y + 0.5;
        uMax = std::max(uMax, Fx / (2.0 * nu) * Y * (H - Y));
    }
    for (std::int64_t y = 0; y < ny; ++y) {
        const double Y = y + 0.5;
        const double expected = Fx / (2.0 * nu) * Y * (H - Y);
        linf = std::max(linf, std::fabs(ux[static_cast<std::size_t>(y)] - expected) / uMax);
    }
    std::printf("external_force: u_max=%.4e Linf(rel)=%.4f\n", uMax, linf);
    CF_CHECK(linf < 0.02);

    // Zero external force -> stays at rest (force really is read from the field).
    const auto uxRest = runChannel(nx, ny, omega, 0.0, 2000);
    double restMax = 0.0;
    for (double v : uxRest) restMax = std::max(restMax, std::fabs(v));
    CF_CHECK(restMax < 1e-12);

    if (cftest::failures == 0) std::printf("external_force: all checks passed\n");
    return cftest::failures;
}
