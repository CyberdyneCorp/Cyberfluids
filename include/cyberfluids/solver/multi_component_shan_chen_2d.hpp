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
#include "cyberfluids/timestep/evolve.hpp"
#include "cyberfluids/timestep/shan_chen.hpp"

namespace cyberfluids::solver {

/// Two-component (immiscible "oil + water") Shan-Chen model on a fully periodic
/// 2D D2Q9 box. Two ForcedD2Q9 fluid lattices, each with its own
/// ExternalForceBGKdynamics and density field, stepped in lockstep:
///   compute rho_A, rho_B  ->  cross force on A (from rho_B) and on B (from
///   rho_A)  ->  collide + periodic stream both.
/// Pseudopotential psi = rho, weights w_i = t_i, single repulsive coupling G>0.
/// Above a critical G the two species spontaneously de-mix. Reuses
/// computeDensityField and applyInterComponentForce. See physical-models spec.
template <class Backend = backend::Default, class T = double>
class MultiComponentShanChen2D {
    using D = descriptors::ForcedD2Q9;
    using Dyn = ExternalForceBGKdynamics<T, D>;

public:
    /// `omega` is shared by both species (equal viscosity/relaxation is the
    /// cleanest symmetric de-mixing case). `G` is the inter-species repulsion.
    MultiComponentShanChen2D(std::int64_t nx, std::int64_t ny, T omega, T G)
        : nx_(nx),
          ny_(ny),
          G_(G),
          a_(nx, ny),
          b_(nx, ny),
          ascratch_(nx * ny),
          bscratch_(nx * ny),
          rhoA_(static_cast<std::size_t>(nx * ny)),
          rhoB_(static_cast<std::size_t>(nx * ny)),
          adyn_(std::make_shared<Dyn>(omega)),
          bdyn_(std::make_shared<Dyn>(omega)) {
        a_.attributeDynamics(a_.getBoundingBox(), adyn_);
        b_.attributeDynamics(b_.getBoundingBox(), bdyn_);
        // Default seed in the validated stable window (mean 0.7). De-mixing needs
        // G*mean >~ 1 (e.g. G=1.8); G*mean <~ 1 (e.g. G=1.2) stays homogeneous.
        initMixed(T(0.7), T(0.035));
    }

    /// Seed a nearly-homogeneous mixture: both species at `mean` with small,
    /// anti-correlated noise (rho_A up where rho_B is down), so total density is
    /// ~uniform and the symmetry-breaking triggers spinodal de-mixing.
    void initMixed(T mean, T noiseAmplitude) {
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y) {
                const std::int64_t c = a_.index(x, y);
                const T n = noiseAmplitude * hashNoise(x, y);
                setCell(a_, c, mean + n);
                setCell(b_, c, mean - n);
            }
    }

    /// Seed a sharp initial interface: species A fills the left half, species B
    /// the right half (each at `mean`, trace `minority` of the other). Useful to
    /// study interface stability / surface tension rather than de-mixing onset.
    void initSplit(T mean, T minority) {
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y) {
                const std::int64_t c = a_.index(x, y);
                const bool leftHalf = x < nx_ / 2;
                setCell(a_, c, leftHalf ? mean : minority);
                setCell(b_, c, leftHalf ? minority : mean);
            }
    }

    void step() {
        // Both densities first (each cross force reads the OTHER species at the
        // same pre-collision time level).
        computeDensityField<Backend>(a_, rhoA_.data());
        computeDensityField<Backend>(b_, rhoB_.data());
        applyInterComponentForce<Backend>(a_, rhoA_.data(), rhoB_.data(), G_);  // A repelled by B
        applyInterComponentForce<Backend>(b_, rhoB_.data(), rhoA_.data(), G_);  // B repelled by A
        cyberfluids::collide<Backend>(a_);
        cyberfluids::streamPeriodic<Backend>(a_, ascratch_);
        cyberfluids::collide<Backend>(b_);
        cyberfluids::streamPeriodic<Backend>(b_, bscratch_);
    }
    void run(std::int64_t steps) {
        for (std::int64_t s = 0; s < steps; ++s) step();
    }

    std::int64_t nx() const { return nx_; }
    std::int64_t ny() const { return ny_; }

    T densityA(std::int64_t x, std::int64_t y) { return adyn_->computeDensity(a_.get(x, y)); }
    T densityB(std::int64_t x, std::int64_t y) { return bdyn_->computeDensity(b_.get(x, y)); }

    /// Per-species conserved mass (must stay constant: streaming is periodic and
    /// the Guo forcing preserves the zeroth moment).
    std::array<T, 2> totalMass() {
        T mA = T(0), mB = T(0);
        for (std::int64_t c = 0; c < a_.ncells(); ++c)
            for (int i = 0; i < D::q; ++i) {
                mA += a_.populations()(i, c);
                mB += b_.populations()(i, c);
            }
        return {mA, mB};
    }

    /// De-mixing order parameter: mean of |rho_A - rho_B| / (rho_A + rho_B) over
    /// the domain. ~0 for a homogeneous mixture, approaches 1 as the species
    /// separate into nearly pure domains. A clean scalar for the de-mix test.
    T segregation() {
        T sum = T(0);
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y) {
                const T ra = densityA(x, y), rb = densityB(x, y);
                const T tot = ra + rb;
                if (tot > T(0)) sum += std::abs(ra - rb) / tot;
            }
        return sum / static_cast<T>(nx_ * ny_);
    }

private:
    static void setCell(BlockLattice2D<T, D>& lat, std::int64_t c, T rho) {
        for (int i = 0; i < D::q; ++i)
            lat.populations()(i, c) = BGKdynamics<T, D>::equilibrium(i, rho, {T(0), T(0)}, T(0));
    }

    // Deterministic pseudo-noise in [-1, 1] (reproducible symmetry-breaking).
    static T hashNoise(std::int64_t x, std::int64_t y) {
        const T h = std::sin(static_cast<T>(x) * T(127.1) + static_cast<T>(y) * T(311.7)) *
                    T(43758.5453);
        return T(2) * (h - std::floor(h)) - T(1);
    }

    std::int64_t nx_, ny_;
    T G_;
    BlockLattice2D<T, D> a_, b_;
    PopulationField<T, D> ascratch_, bscratch_;
    std::vector<T> rhoA_, rhoB_;
    std::shared_ptr<Dyn> adyn_, bdyn_;
};

}  // namespace cyberfluids::solver
