#pragma once

#include <array>

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/dynamics.hpp"
#include "cyberfluids/core/external_traits.hpp"
#include "cyberfluids/dynamics/bgk.hpp"

namespace cyberfluids {

/// Partial bounce-back (porous media) collision on a descriptor carrying a
/// per-cell solid fraction `ns in [0,1]` (e.g. descriptors::PorousD2Q9). A
/// uniform Guo body force is a member; the solid fraction is read per cell from
/// the external field. The Walsh, Burr & Holmes (2009) linear blend is applied
/// in its algebraically-equivalent convex form:
///
///   f_i^out = (1 - ns) * f_i^{BGK+force,out} + ns * f_opposite(i)
///
/// where f_i^{BGK+force,out} = f_i - omega (f_i - feq_i) + S_i is the ordinary
/// forced-BGK post-collision population and f_opposite(i) is the *pre*-collision
/// population streaming back (node bounce-back). This makes:
///   ns = 0  ->  exactly forced BGK (and, with force = 0, exactly plain BGK);
///   ns = 1  ->  exactly the population swap f_i <- f_opposite(i) (no-slip node).
/// The body force is attenuated by (1 - ns) so it vanishes in solid cells.
/// See openspec/specs/physical-models/spec.md.
template <class T, LatticeDescriptor Descriptor>
class PorousForcedBGKdynamics final : public Dynamics<T, Descriptor> {
    static_assert(numScalarScalars<Descriptor>() >= 1,
                  "PorousForcedBGKdynamics needs a descriptor declaring an ext::Scalar "
                  "solid-fraction slot (e.g. descriptors::PorousD2Q9)");

public:
    using Base = Dynamics<T, Descriptor>;
    using Velocity = typename Base::Velocity;
    static constexpr int q = Descriptor::q;
    static constexpr int d = Descriptor::d;
    static constexpr int nsOffset = Descriptor::ExternalSpec::scalarBeginsAt;

    explicit PorousForcedBGKdynamics(T omega, const Velocity& force = {})
        : omega_(omega), force_(force) {}

    T getOmega() const override { return omega_; }
    void setOmega(T omega) override { omega_ = omega; }
    const Velocity& force() const { return force_; }
    void setForce(const Velocity& f) { force_ = f; }

    static T kinematicViscosity(T omega) {
        return BGKdynamics<T, Descriptor>::kinematicViscosity(omega);
    }

    T computeEquilibrium(int iPop, T rho, const Velocity& u, T uSqr) const override {
        return BGKdynamics<T, Descriptor>::equilibrium(iPop, rho, u, uSqr);
    }
    T computeDensity(const Cell<T, Descriptor>& cell) const override {
        T rho = T(0);
        for (int i = 0; i < q; ++i) rho += cell[i];
        return rho;
    }
    /// Physical (Guo half-force) velocity. In a solid cell (ns = 1) the true
    /// velocity is zero; this returns the raw half-force value, so callers that
    /// want the through-flow of a porous cell should use the flux velocity
    /// (sum_i f_i c_i / rho) instead.
    void computeVelocity(const Cell<T, Descriptor>& cell, Velocity& u) const override {
        T rho = T(0);
        Velocity j{};
        for (int i = 0; i < q; ++i) {
            const T fi = cell[i];
            rho += fi;
            for (int a = 0; a < d; ++a) j[a] += fi * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        for (int a = 0; a < d; ++a) u[a] = (j[a] + T(0.5) * force_[a]) * invRho;
    }

    void collide(Cell<T, Descriptor>& cell) const override {
        // Snapshot pre-collision populations: the bounce-back term reads
        // f_opposite(i), which must be the original value, not one already
        // overwritten by an earlier iteration of the write loop.
        std::array<T, q> f{};
        T rho = T(0);
        Velocity j{};
        for (int i = 0; i < q; ++i) {
            f[i] = cell[i];
            rho += f[i];
            for (int a = 0; a < d; ++a) j[a] += f[i] * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        Velocity u{};
        T uSqr = T(0);
        for (int a = 0; a < d; ++a) {
            u[a] = (j[a] + T(0.5) * force_[a]) * invRho;  // Guo half-force velocity
            uSqr += u[a] * u[a];
        }

        const T ns = cell.external(nsOffset);
        const T fluid = T(1) - ns;
        const T invCs2 = static_cast<T>(Descriptor::invCs2);
        const T prefactor = T(1) - T(0.5) * omega_;
        for (int i = 0; i < q; ++i) {
            const T feq = BGKdynamics<T, Descriptor>::equilibrium(i, rho, u, uSqr);
            T ciu = T(0);
            for (int a = 0; a < d; ++a) ciu += static_cast<T>(Descriptor::c[i][a]) * u[a];
            T source = T(0);
            for (int a = 0; a < d; ++a) {
                const T ci = static_cast<T>(Descriptor::c[i][a]);
                source += ((ci - u[a]) * invCs2 + ciu * invCs2 * invCs2 * ci) * force_[a];
            }
            source *= prefactor * static_cast<T>(Descriptor::t[i]);
            const T bgkOut = f[i] - omega_ * (f[i] - feq) + source;
            cell[i] = fluid * bgkOut + ns * f[Descriptor::opposite[i]];
        }
    }

private:
    T omega_;
    Velocity force_;
};

}  // namespace cyberfluids
