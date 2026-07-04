#pragma once

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
#include "cyberfluids/physics/shan_chen_eos.hpp"
#include "cyberfluids/timestep/evolve.hpp"
#include "cyberfluids/timestep/shan_chen.hpp"

namespace cyberfluids::solver {

/// Single-component Shan-Chen multiphase on a fully periodic 2D D2Q9 box.
/// Each step: density field -> non-local interaction force (into the per-cell
/// external force) -> collide (ExternalForceBGKdynamics/Guo) -> periodic stream.
/// See openspec/specs/physical-models/spec.md.
template <class Backend = backend::Default, class T = double>
class ShanChen2D {
    using D = descriptors::ForcedD2Q9;
    using Dyn = ExternalForceBGKdynamics<T, D>;

public:
    ShanChen2D(std::int64_t nx, std::int64_t ny, T omega, ShanChenParameters<T> params)
        : nx_(nx),
          ny_(ny),
          params_(params),
          lattice_(nx, ny),
          scratch_(nx * ny),
          rho_(static_cast<std::size_t>(nx * ny)),
          dyn_(std::make_shared<Dyn>(omega)) {
        lattice_.attributeDynamics(lattice_.getBoundingBox(), dyn_);
        initDensity(params.rho0, T(0));
    }

    /// Initialize at rest with density = mean + deterministic small noise.
    void initDensity(T mean, T noiseAmplitude) {
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y) {
                const std::int64_t c = lattice_.index(x, y);
                const T rho = mean + noiseAmplitude * hashNoise(x, y);
                for (int i = 0; i < D::q; ++i)
                    lattice_.populations()(i, c) =
                        BGKdynamics<T, D>::equilibrium(i, rho, {T(0), T(0)}, T(0));
            }
    }

    /// Initialize a circular liquid droplet (radius R, centered) in vapour.
    void initDroplet(T rhoLiquid, T rhoVapour, T radius) {
        const T cx = T(0.5) * nx_, cy = T(0.5) * ny_;
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y) {
                const std::int64_t c = lattice_.index(x, y);
                const T dx = x - cx, dy = y - cy;
                const T r = std::sqrt(dx * dx + dy * dy);
                const T rho = (r < radius) ? rhoLiquid : rhoVapour;
                for (int i = 0; i < D::q; ++i)
                    lattice_.populations()(i, c) =
                        BGKdynamics<T, D>::equilibrium(i, rho, {T(0), T(0)}, T(0));
            }
    }

    void step() {
        computeDensityField<Backend>(lattice_, rho_.data());
        applyShanChenForce<Backend>(lattice_, rho_.data(), params_);
        cyberfluids::collide<Backend>(lattice_);
        cyberfluids::streamPeriodic<Backend>(lattice_, scratch_);
    }
    void run(std::int64_t steps) {
        for (std::int64_t s = 0; s < steps; ++s) step();
    }

    std::int64_t nx() const { return nx_; }
    std::int64_t ny() const { return ny_; }

    T density(std::int64_t x, std::int64_t y) {
        auto cell = lattice_.get(x, y);
        return dyn_->computeDensity(cell);
    }

    std::array<T, 2> minMaxDensity() {
        T lo = density(0, 0), hi = lo;
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y) {
                const T r = density(x, y);
                lo = (r < lo) ? r : lo;
                hi = (r > hi) ? r : hi;
            }
        return {lo, hi};
    }

    T totalMass() {
        T m = T(0);
        for (std::int64_t c = 0; c < lattice_.ncells(); ++c)
            for (int i = 0; i < D::q; ++i) m += lattice_.populations()(i, c);
        return m;
    }

private:
    // Deterministic pseudo-noise in [-1, 1] (reproducible symmetry-breaking).
    static T hashNoise(std::int64_t x, std::int64_t y) {
        const T h = std::sin(static_cast<T>(x) * T(127.1) + static_cast<T>(y) * T(311.7)) *
                    T(43758.5453);
        return T(2) * (h - std::floor(h)) - T(1);
    }

    std::int64_t nx_, ny_;
    ShanChenParameters<T> params_;
    BlockLattice2D<T, D> lattice_;
    PopulationField<T, D> scratch_;
    std::vector<T> rho_;
    std::shared_ptr<Dyn> dyn_;
};

}  // namespace cyberfluids::solver
