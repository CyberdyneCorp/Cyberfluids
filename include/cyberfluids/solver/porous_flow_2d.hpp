#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/dynamics/porous.hpp"
#include "cyberfluids/timestep/evolve.hpp"

namespace cyberfluids::solver {

/// Force-driven flow through a uniform porous medium on D2Q9: fully periodic in
/// both directions, driven by a uniform body force F = (Fx, 0), with a per-cell
/// solid fraction `ns` handled by partial bounce-back (PorousForcedBGKdynamics).
///
/// For a uniform ns the steady state is a collision fixed point whose first
/// moment gives the analytic Darcy through-flow (flux velocity):
///
///   U_flux = (sum_i f_i c_i)_x / rho = (1 - ns) * Fx / (2 * ns * rho)
///
/// which is linear in Fx (Darcy's law), decreases monotonically with ns, and
/// yields an apparent permeability k = nu * U_flux / Fx = nu (1 - ns) / (2 ns).
/// This is a self-contained analytic oracle — no external reference needed.
template <class Backend = backend::Default, class T = double>
class PorousBox2D {
    using D = descriptors::PorousD2Q9;

public:
    PorousBox2D(std::int64_t nx, std::int64_t ny, T omega, T forceX)
        : nx_(nx),
          ny_(ny),
          forceX_(forceX),
          lattice_(nx, ny),
          scratch_(nx * ny),
          dyn_(std::make_shared<PorousForcedBGKdynamics<T, D>>(omega,
                                                               std::array<T, 2>{forceX, T(0)})) {
        lattice_.attributeDynamics(lattice_.getBoundingBox(), dyn_);
        initEquilibrium();
    }

    void initEquilibrium() {
        for (std::int64_t c = 0; c < lattice_.ncells(); ++c)
            for (int i = 0; i < D::q; ++i)
                lattice_.populations()(i, c) =
                    BGKdynamics<T, D>::equilibrium(i, T(1), {T(0), T(0)}, T(0));
    }

    /// Uniform solid fraction across the whole box.
    void setUniformSolidFraction(T ns) {
        auto& ext = lattice_.externalField();
        for (std::int64_t c = 0; c < lattice_.ncells(); ++c)
            ext(PorousForcedBGKdynamics<T, D>::nsOffset, c) = ns;
    }

    /// Per-cell solid fraction ns = f(x, y). Enables graded / patterned media.
    void setSolidFraction(const std::function<T(std::int64_t, std::int64_t)>& f) {
        auto& ext = lattice_.externalField();
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y)
                ext(PorousForcedBGKdynamics<T, D>::nsOffset, x * ny_ + y) = f(x, y);
    }

    void step() { cyberfluids::collideAndStream<Backend>(lattice_, scratch_); }
    void run(std::int64_t steps) {
        for (std::int64_t s = 0; s < steps; ++s) step();
    }

    std::int64_t nx() const { return nx_; }
    std::int64_t ny() const { return ny_; }
    BlockLattice2D<T, D>& lattice() { return lattice_; }

    /// Mean streamwise flux velocity (sum_i f_i c_ix / rho) averaged over cells.
    /// This is the through-flow: it is 0 in fully-solid cells, unlike the Guo
    /// half-force velocity which reads Fx/2 there.
    T meanFluxVelocityX() {
        auto& pop = lattice_.populations();
        T sum = T(0);
        for (std::int64_t c = 0; c < lattice_.ncells(); ++c) {
            T rho = T(0), jx = T(0);
            for (int i = 0; i < D::q; ++i) {
                const T fi = pop(i, c);
                rho += fi;
                jx += fi * static_cast<T>(D::c[i][0]);
            }
            sum += jx / rho;
        }
        return sum / static_cast<T>(lattice_.ncells());
    }

    /// Analytic steady flux velocity for a uniform ns (rho = 1).
    T analyticFluxVelocityX(T ns) const {
        return (T(1) - ns) * forceX_ / (T(2) * ns);
    }

    static T kinematicViscosity(T omega) {
        return PorousForcedBGKdynamics<T, D>::kinematicViscosity(omega);
    }

private:
    std::int64_t nx_, ny_;
    T forceX_;
    BlockLattice2D<T, D> lattice_;
    PopulationField<T, D> scratch_;
    std::shared_ptr<PorousForcedBGKdynamics<T, D>> dyn_;
};

}  // namespace cyberfluids::solver
