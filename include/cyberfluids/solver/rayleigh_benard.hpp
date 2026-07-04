#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/boundary/bounce_back.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/dynamics/advection_diffusion.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/dynamics/forced.hpp"
#include "cyberfluids/io/vtk_writer.hpp"
#include "cyberfluids/timestep/buoyancy.hpp"
#include "cyberfluids/timestep/coupling.hpp"
#include "cyberfluids/timestep/evolve.hpp"

namespace cyberfluids::solver {

/// 2D Rayleigh-Bénard convection: a ForcedD2Q9 fluid (no-slip y-walls, periodic
/// x) two-way coupled to an AdvectedD2Q5 temperature (hot bottom / cold top
/// Dirichlet walls). Each step: fluid velocity advects temperature, temperature
/// drives a buoyancy body force on the fluid. See physical-models spec.
template <class Backend = backend::Default, class T = double>
class RayleighBenard2D {
    using FluidD = descriptors::ForcedD2Q9;
    using TempD = descriptors::AdvectedD2Q5;
    using FluidDyn = ExternalForceBGKdynamics<T, FluidD>;
    using TempDyn = AdvectionDiffusionBGKdynamics<T, TempD>;

public:
    RayleighBenard2D(std::int64_t nx, std::int64_t ny, T omegaFluid, T omegaTemp,
                     BoussinesqParameters<T> params, T tHot, T tCold)
        : nx_(nx),
          ny_(ny),
          tHot_(tHot),
          tCold_(tCold),
          params_(params),
          fluid_(nx, ny),
          temp_(nx, ny),
          fscratch_(nx * ny),
          tscratch_(nx * ny),
          fdyn_(std::make_shared<FluidDyn>(omegaFluid)),
          tdyn_(std::make_shared<TempDyn>(omegaTemp)) {
        fluid_.attributeDynamics(fluid_.getBoundingBox(), fdyn_);
        temp_.attributeDynamics(temp_.getBoundingBox(), tdyn_);
        initConductive();
    }

    /// Build from dimensionless parameters. Fixes the free-fall velocity U_f, the
    /// plate spacing H = ny, and Pr; derives nu, kappa (via omega) and g so that
    /// only omega changes with Ra (a clean sub/super-critical A/B). tHot=1 at the
    /// bottom, tCold=0 at the top.
    static RayleighBenard2D fromDimensionless(std::int64_t nx, std::int64_t ny, T Ra, T Pr, T Uf) {
        const T H = static_cast<T>(ny);
        const T nu = Uf * H / std::sqrt(Ra / Pr);
        const T kappa = nu / Pr;
        const T omegaFluid = T(1) / (nu * T(3) + T(0.5));
        const T omegaTemp = T(1) / (kappa * T(3) + T(0.5));
        const T tHot = T(1), tCold = T(0), dT = tHot - tCold;
        BoussinesqParameters<T> p;
        p.gravity = Uf * Uf / (H * dT);  // g*beta*dT = U_f^2 / H, with beta=1
        p.thermalExpansion = T(1);
        p.referenceTemperature = T(0.5) * (tHot + tCold);
        p.gravityAxis = 1;
        return RayleighBenard2D(nx, ny, omegaFluid, omegaTemp, p, tHot, tCold);
    }

    /// Linear conduction temperature profile, fluid at rest.
    void initConductive() {
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y) {
                const std::int64_t c = temp_.index(x, y);
                const T tlin = conductionProfile(y);
                for (int i = 0; i < TempD::q; ++i)
                    temp_.populations()(i, c) = TempDyn::equilibrium(i, tlin, {T(0), T(0)});
                for (int i = 0; i < FluidD::q; ++i)
                    fluid_.populations()(i, c) =
                        BGKdynamics<T, FluidD>::equilibrium(i, T(1), {T(0), T(0)}, T(0));
            }
    }

    /// Seed a small temperature perturbation to break symmetry (triggers onset).
    void perturbTemperature(T amplitude) {
        constexpr T PI = T(3.14159265358979323846);
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y) {
                const std::int64_t c = temp_.index(x, y);
                const T pert = amplitude * std::sin(T(2) * PI * x / nx_) *
                               std::sin(PI * (y + T(0.5)) / ny_);
                const T phi = conductionProfile(y) + pert;
                for (int i = 0; i < TempD::q; ++i)
                    temp_.populations()(i, c) = TempDyn::equilibrium(i, phi, {T(0), T(0)});
            }
    }

    void step() {
        // Apply buoyancy first so the advection velocity's Guo half-force uses
        // the current-step force, not the previous step's.
        applyBuoyancy<Backend>(fluid_, temp_, params_);  // temperature(t) -> fluid buoyancy force
        copyVelocityToExternal<Backend>(fluid_, temp_);  // fluid velocity(t) -> temp advection
        cyberfluids::collide<Backend>(fluid_);
        streamFluid();
        std::swap(fluid_.populations(), fscratch_);
        cyberfluids::collide<Backend>(temp_);
        streamTemperature();
        std::swap(temp_.populations(), tscratch_);
    }
    void run(std::int64_t steps) {
        for (std::int64_t s = 0; s < steps; ++s) step();
    }

    std::int64_t nx() const { return nx_; }
    std::int64_t ny() const { return ny_; }

    T temperatureAt(std::int64_t x, std::int64_t y) {
        auto cell = temp_.get(x, y);
        return tdyn_->computeDensity(cell);
    }
    std::array<T, 2> velocityAt(std::int64_t x, std::int64_t y) {
        auto cell = fluid_.get(x, y);
        std::array<T, 2> u{};
        fdyn_->computeVelocity(cell, u);
        return u;
    }

    /// Mean kinetic energy density 0.5 (ux^2 + uy^2) over the domain.
    T avgKineticEnergy() {
        T sum = T(0);
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y) {
                const auto u = velocityAt(x, y);
                sum += T(0.5) * (u[0] * u[0] + u[1] * u[1]);
            }
        return sum / static_cast<T>(nx_ * ny_);
    }

    /// Horizontally averaged temperature at row y.
    T horizontallyAveragedT(std::int64_t y) {
        T sum = T(0);
        for (std::int64_t x = 0; x < nx_; ++x) sum += temperatureAt(x, y);
        return sum / static_cast<T>(nx_);
    }

    /// Analytic pure-conduction profile (half-node walls at y=-1/2 and y=ny-1/2).
    T conductionProfile(std::int64_t y) const {
        return tHot_ + (tCold_ - tHot_) * (static_cast<T>(y) + T(0.5)) / static_cast<T>(ny_);
    }

    void writeVtk(const std::string& path) {
        io::VtkStructuredWriter<2> w(nx_, ny_);
        w.addScalar("temperature",
                    [&](std::array<std::int64_t, 2> c) { return temperatureAt(c[0], c[1]); });
        w.addVector("velocity",
                    [&](std::array<std::int64_t, 2> c) { return velocityAt(c[0], c[1]); });
        w.write(path);
    }

private:
    static std::int64_t wrapX(std::int64_t v, std::int64_t nx) { return (v % nx + nx) % nx; }

    void streamFluid() {
        auto& src = fluid_.populations();
        const std::int64_t nx = nx_, ny = ny_;
        Backend::forEachIndex(fluid_.ncells(), [&, nx, ny](std::int64_t c) {
            const std::int64_t y = c % ny, x = c / ny;
            for (int i = 0; i < FluidD::q; ++i) {
                const std::int64_t sx = wrapX(x - FluidD::c[i][0], nx);
                const std::int64_t sy = y - FluidD::c[i][1];
                if (sy >= 0 && sy < ny)
                    fscratch_(i, c) = src(i, sx * ny + sy);
                else
                    fscratch_(i, c) = boundary::noSlipReflected(src, i, c);  // no-slip walls
            }
        });
    }

    void streamTemperature() {
        auto& src = temp_.populations();
        const std::int64_t nx = nx_, ny = ny_;
        const T tHot = tHot_, tCold = tCold_;
        Backend::forEachIndex(temp_.ncells(), [&, nx, ny, tHot, tCold](std::int64_t c) {
            const std::int64_t y = c % ny, x = c / ny;
            for (int i = 0; i < TempD::q; ++i) {
                const std::int64_t sx = wrapX(x - TempD::c[i][0], nx);
                const std::int64_t sy = y - TempD::c[i][1];
                if (sy < 0)
                    tscratch_(i, c) = boundary::antiBounceBackScalar(src, i, c, tHot);   // bottom
                else if (sy >= ny)
                    tscratch_(i, c) = boundary::antiBounceBackScalar(src, i, c, tCold);  // top
                else
                    tscratch_(i, c) = src(i, sx * ny + sy);
            }
        });
    }

    std::int64_t nx_, ny_;
    T tHot_, tCold_;
    BoussinesqParameters<T> params_;
    BlockLattice2D<T, FluidD> fluid_;
    BlockLattice2D<T, TempD> temp_;
    PopulationField<T, FluidD> fscratch_;
    PopulationField<T, TempD> tscratch_;
    std::shared_ptr<FluidDyn> fdyn_;
    std::shared_ptr<TempDyn> tdyn_;
};

}  // namespace cyberfluids::solver
