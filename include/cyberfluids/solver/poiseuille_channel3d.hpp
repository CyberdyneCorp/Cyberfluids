#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/boundary/bounce_back.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/dynamics/mrt3d.hpp"
#include "cyberfluids/timestep/evolve.hpp"

namespace cyberfluids::solver {

/// Force-driven plane Poiseuille flow on D3Q19: periodic in x (flow direction)
/// and z (spanwise), no-slip bounce-back walls at y = 0 and y = ny-1, driven by
/// a uniform body force F = (Fx, 0, 0). At steady state the streamwise profile
/// is the analytic parabola u(Y) = Fx/(2 rho nu) Y (H - Y), with halfway
/// bounce-back placing the walls at Y = 0 and Y = H = ny (fluid node y at
/// Y = y + 1/2). Templated on the dynamics so the same channel validates both
/// the forced-MRT and forced-BGK D3Q19 operators.
template <class Dyn, class Backend = backend::Default, class T = double>
class PoiseuilleChannel3D {
    using D = descriptors::D3Q19;

public:
    PoiseuilleChannel3D(std::int64_t nx, std::int64_t ny, std::int64_t nz, T omega, T forceX)
        : nx_(nx),
          ny_(ny),
          nz_(nz),
          forceX_(forceX),
          lattice_(nx, ny, nz),
          scratch_(nx * ny * nz),
          dyn_(std::make_shared<Dyn>(omega, std::array<T, 3>{forceX, T(0), T(0)})) {
        lattice_.attributeDynamics(lattice_.getBoundingBox(), dyn_);
        initEquilibrium();
    }

    void initEquilibrium() {
        for (std::int64_t c = 0; c < lattice_.ncells(); ++c)
            for (int i = 0; i < D::q; ++i)
                lattice_.populations()(i, c) =
                    BGKdynamics<T, D>::equilibrium(i, T(1), {T(0), T(0), T(0)}, T(0));
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
    std::int64_t nz() const { return nz_; }

    std::array<T, 3> velocity(std::int64_t x, std::int64_t y, std::int64_t z) {
        auto cell = lattice_.get(x, y, z);
        std::array<T, 3> u{};
        dyn_->computeVelocity(cell, u);  // includes the Guo half-force correction
        return u;
    }

    /// Analytic steady-state streamwise velocity at fluid node y.
    T analyticUx(std::int64_t y, T omega) const {
        const T nu = Dyn::kinematicViscosity(omega);
        const T H = static_cast<T>(ny_);
        const T Y = static_cast<T>(y) + T(0.5);
        return forceX_ / (T(2) * T(1) * nu) * Y * (H - Y);  // rho = 1
    }

private:
    void streamWithBoundaries() {
        auto& src = lattice_.populations();
        const std::int64_t nx = nx_, ny = ny_, nz = nz_;
        Backend::forEachIndex(lattice_.ncells(), [&, nx, ny, nz](std::int64_t c) {
            const std::int64_t z = c % nz;
            const std::int64_t y = (c / nz) % ny;
            const std::int64_t x = c / (ny * nz);
            for (int i = 0; i < D::q; ++i) {
                const std::int64_t sx = ((x - D::c[i][0]) % nx + nx) % nx;  // periodic x
                const std::int64_t sy = y - D::c[i][1];                     // walls in y
                const std::int64_t sz = ((z - D::c[i][2]) % nz + nz) % nz;  // periodic z
                if (sy >= 0 && sy < ny)
                    scratch_(i, c) = src(i, (sx * ny + sy) * nz + sz);
                else
                    scratch_(i, c) = boundary::noSlipReflected(src, i, c);  // no-slip y-walls
            }
        });
    }

    std::int64_t nx_, ny_, nz_;
    T forceX_;
    BlockLattice3D<T, D> lattice_;
    PopulationField<T, D> scratch_;
    std::shared_ptr<Dyn> dyn_;
};

}  // namespace cyberfluids::solver
