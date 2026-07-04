#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <utility>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/boundary/bounce_back.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/timestep/evolve.hpp"

namespace cyberfluids::solver {

/// 2D lid-driven cavity (Navier-Stokes) on D2Q9: BGK bulk, no-slip bounce-back
/// on the left/right/bottom walls, and a moving top lid via bounce-back with
/// momentum. Step = collide-in-place, then a pull-stream that applies the
/// boundary rules where the pull source leaves the domain.
///
/// The lid uses moving-wall bounce-back rather than Zou/He because it conserves
/// mass exactly in a closed cavity (the injected mass 2 rho invCs2 U (w8 - w7)
/// vanishes since w7 == w8), which matters for tight steady-state oracle
/// comparisons. Zou/He is implemented and unit-tested as a velocity boundary
/// (boundary::zouHeVelocityTop). See physical-models and boundary-conditions specs.
template <class Backend = backend::Default, class T = double>
class LidDrivenCavity2D {
    using D = descriptors::D2Q9;

public:
    LidDrivenCavity2D(std::int64_t nx, std::int64_t ny, T omega, T lidVelocity)
        : nx_(nx),
          ny_(ny),
          lidU_(lidVelocity),
          lattice_(nx, ny),
          scratch_(nx * ny),
          bgk_(std::make_shared<BGKdynamics<T, D>>(omega)) {
        lattice_.attributeDynamics(lattice_.getBoundingBox(), bgk_);
        initEquilibrium(T(1), {T(0), T(0)});
    }

    void initEquilibrium(T rho, std::array<T, 2> u) {
        const T uSqr = u[0] * u[0] + u[1] * u[1];
        for (std::int64_t c = 0; c < lattice_.ncells(); ++c)
            for (int i = 0; i < D::q; ++i)
                lattice_.populations()(i, c) = BGKdynamics<T, D>::equilibrium(i, rho, u, uSqr);
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

    T density(std::int64_t x, std::int64_t y) {
        auto cell = lattice_.get(x, y);
        return bgk_->computeDensity(cell);
    }
    std::array<T, 2> velocity(std::int64_t x, std::int64_t y) {
        auto cell = lattice_.get(x, y);
        std::array<T, 2> u{};
        bgk_->computeVelocity(cell, u);
        return u;
    }

    T totalMass() {
        T m = T(0);
        for (std::int64_t c = 0; c < lattice_.ncells(); ++c)
            for (int i = 0; i < D::q; ++i) m += lattice_.populations()(i, c);
        return m;
    }

    /// Dump the two centerline velocity profiles as CSV (steady-state analysis).
    void writeCenterlines(const std::string& path) {
        std::ofstream out(path);
        out << "axis,coord,ux,uy\n";
        const std::int64_t xc = nx_ / 2, yc = ny_ / 2;
        for (std::int64_t y = 0; y < ny_; ++y) {
            auto u = velocity(xc, y);
            out << "vertical," << y << "," << u[0] << "," << u[1] << "\n";
        }
        for (std::int64_t x = 0; x < nx_; ++x) {
            auto u = velocity(x, yc);
            out << "horizontal," << x << "," << u[0] << "," << u[1] << "\n";
        }
    }

private:
    void streamWithBoundaries() {
        auto& src = lattice_.populations();
        const std::int64_t nx = nx_, ny = ny_;
        const std::array<T, 2> uWall{lidU_, T(0)};
        Backend::forEachIndex(lattice_.ncells(), [&, nx, ny](std::int64_t c) {
            const std::int64_t y = c % ny;
            const std::int64_t x = c / ny;
            const bool topInterior = (y == ny - 1) && x > 0 && x < nx - 1;

            for (int i = 0; i < D::q; ++i) {
                const std::int64_t sx = x - D::c[i][0];
                const std::int64_t sy = y - D::c[i][1];
                if (sx >= 0 && sx < nx && sy >= 0 && sy < ny) {
                    scratch_(i, c) = src(i, sx * ny + sy);
                } else if (topInterior && D::c[i][1] == -1) {
                    // Lid-facing population: moving-wall bounce-back (mass-conserving).
                    scratch_(i, c) = boundary::noSlipReflected(src, i, c) +
                                     boundary::movingWallTerm<T, D, 2>(i, T(1), uWall);
                } else {
                    scratch_(i, c) = boundary::noSlipReflected(src, i, c);  // no-slip walls
                }
            }
        });
    }

    std::int64_t nx_, ny_;
    T lidU_;
    BlockLattice2D<T, D> lattice_;
    PopulationField<T, D> scratch_;
    std::shared_ptr<BGKdynamics<T, D>> bgk_;
};

/// 3D lid-driven cavity on D3Q19: BGK bulk, no-slip bounce-back on five walls,
/// and a moving lid on the top z-face (velocity +x) via bounce-back with
/// momentum. See physical-models and boundary-conditions specs.
template <class Backend = backend::Default, class T = double>
class LidDrivenCavity3D {
    using D = descriptors::D3Q19;

public:
    LidDrivenCavity3D(std::int64_t nx, std::int64_t ny, std::int64_t nz, T omega, T lidVelocity)
        : nx_(nx),
          ny_(ny),
          nz_(nz),
          lidU_(lidVelocity),
          lattice_(nx, ny, nz),
          scratch_(nx * ny * nz),
          bgk_(std::make_shared<BGKdynamics<T, D>>(omega)) {
        lattice_.attributeDynamics(lattice_.getBoundingBox(), bgk_);
        initEquilibrium(T(1), {T(0), T(0), T(0)});
    }

    void initEquilibrium(T rho, std::array<T, 3> u) {
        const T uSqr = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
        for (std::int64_t c = 0; c < lattice_.ncells(); ++c)
            for (int i = 0; i < D::q; ++i)
                lattice_.populations()(i, c) = BGKdynamics<T, D>::equilibrium(i, rho, u, uSqr);
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

    T density(std::int64_t x, std::int64_t y, std::int64_t z) {
        auto cell = lattice_.get(x, y, z);
        return bgk_->computeDensity(cell);
    }
    std::array<T, 3> velocity(std::int64_t x, std::int64_t y, std::int64_t z) {
        auto cell = lattice_.get(x, y, z);
        std::array<T, 3> u{};
        bgk_->computeVelocity(cell, u);
        return u;
    }

private:
    void streamWithBoundaries() {
        auto& src = lattice_.populations();
        const std::int64_t nx = nx_, ny = ny_, nz = nz_;
        const std::array<T, 3> uWall{lidU_, T(0), T(0)};
        Backend::forEachIndex(lattice_.ncells(), [&, nx, ny, nz](std::int64_t c) {
            const std::int64_t z = c % nz;
            const std::int64_t y = (c / nz) % ny;
            const std::int64_t x = c / (ny * nz);
            const bool topInterior = (z == nz - 1) && x > 0 && x < nx - 1 && y > 0 && y < ny - 1;

            for (int i = 0; i < D::q; ++i) {
                const std::int64_t sx = x - D::c[i][0];
                const std::int64_t sy = y - D::c[i][1];
                const std::int64_t sz = z - D::c[i][2];
                if (sx >= 0 && sx < nx && sy >= 0 && sy < ny && sz >= 0 && sz < nz) {
                    scratch_(i, c) = src(i, (sx * ny + sy) * nz + sz);
                } else if (topInterior && D::c[i][2] == -1) {
                    // Missing lid-facing population: moving-wall bounce-back.
                    scratch_(i, c) = boundary::noSlipReflected(src, i, c) +
                                     boundary::movingWallTerm<T, D, 3>(i, T(1), uWall);
                } else {
                    scratch_(i, c) = boundary::noSlipReflected(src, i, c);  // no-slip walls
                }
            }
        });
    }

    std::int64_t nx_, ny_, nz_;
    T lidU_;
    BlockLattice3D<T, D> lattice_;
    PopulationField<T, D> scratch_;
    std::shared_ptr<BGKdynamics<T, D>> bgk_;
};

}  // namespace cyberfluids::solver
