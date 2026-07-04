#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <utility>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/boundary/bounce_back.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/dynamics/forced.hpp"
#include "cyberfluids/timestep/evolve.hpp"

namespace cyberfluids::solver {

/// Force-driven plane Poiseuille flow on D2Q9: periodic in x (flow direction),
/// no-slip bounce-back walls at y = 0 and y = ny-1, driven by a uniform body
/// force F = (Fx, 0) via Guo forcing. At steady state the profile is the
/// analytic parabola u(Y) = Fx/(2 rho nu) Y (H - Y), with halfway bounce-back
/// placing the walls at Y = 0 and Y = H = ny (fluid node y is at Y = y + 1/2).
template <class Backend = backend::Default, class T = double>
class PoiseuilleChannel2D {
    using D = descriptors::D2Q9;

public:
    PoiseuilleChannel2D(std::int64_t nx, std::int64_t ny, T omega, T forceX)
        : nx_(nx),
          ny_(ny),
          forceX_(forceX),
          lattice_(nx, ny),
          scratch_(nx * ny),
          forced_(std::make_shared<ForcedBGKdynamics<T, D>>(omega,
                                                            std::array<T, 2>{forceX, T(0)})) {
        lattice_.attributeDynamics(lattice_.getBoundingBox(), forced_);
        initEquilibrium();
    }

    void initEquilibrium() {
        for (std::int64_t c = 0; c < lattice_.ncells(); ++c)
            for (int i = 0; i < D::q; ++i)
                lattice_.populations()(i, c) =
                    BGKdynamics<T, D>::equilibrium(i, T(1), {T(0), T(0)}, T(0));
    }

    void step() {
        cyberfluids::collide<Backend>(lattice_);
        streamWithBoundaries();
        std::swap(lattice_.populations(), scratch_);
    }
    void run(std::int64_t steps) {
        for (std::int64_t s = 0; s < steps; ++s) step();
    }

    std::int64_t nx() const { return nx_; }
    std::int64_t ny() const { return ny_; }

    std::array<T, 2> velocity(std::int64_t x, std::int64_t y) {
        auto cell = lattice_.get(x, y);
        std::array<T, 2> u{};
        forced_->computeVelocity(cell, u);  // includes the Guo half-force correction
        return u;
    }

    /// Analytic steady-state streamwise velocity at fluid node y.
    T analyticUx(std::int64_t y, T omega) const {
        const T nu = ForcedBGKdynamics<T, D>::kinematicViscosity(omega);
        const T H = static_cast<T>(ny_);
        const T Y = static_cast<T>(y) + T(0.5);
        return forceX_ / (T(2) * T(1) * nu) * Y * (H - Y);  // rho = 1
    }

private:
    void streamWithBoundaries() {
        auto& src = lattice_.populations();
        const std::int64_t nx = nx_, ny = ny_;
        Backend::forEachIndex(lattice_.ncells(), [&, nx, ny](std::int64_t c) {
            const std::int64_t y = c % ny;
            const std::int64_t x = c / ny;
            for (int i = 0; i < D::q; ++i) {
                const std::int64_t sx = ((x - D::c[i][0]) % nx + nx) % nx;  // periodic in x
                const std::int64_t sy = y - D::c[i][1];
                if (sy >= 0 && sy < ny)
                    scratch_(i, c) = src(i, sx * ny + sy);
                else
                    scratch_(i, c) = boundary::noSlipReflected(src, i, c);  // no-slip y-walls
            }
        });
    }

    std::int64_t nx_, ny_;
    T forceX_;
    BlockLattice2D<T, D> lattice_;
    PopulationField<T, D> scratch_;
    std::shared_ptr<ForcedBGKdynamics<T, D>> forced_;
};

}  // namespace cyberfluids::solver
