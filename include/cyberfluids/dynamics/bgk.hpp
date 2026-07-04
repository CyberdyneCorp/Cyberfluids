#pragma once

#include <array>

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/dynamics.hpp"

namespace cyberfluids {

/// Single-relaxation-time (BGK) collision model. Relaxes populations toward the
/// second-order (O(Ma^2)) equilibrium at rate `omega = 1/tau`; the kinematic
/// viscosity is `nu = cs2 * (1/omega - 1/2)`. Formulation matches the Palabos
/// reference. See openspec/specs/collision-dynamics/spec.md.
template <class T, LatticeDescriptor Descriptor>
class BGKdynamics final : public Dynamics<T, Descriptor> {
public:
    using Base = Dynamics<T, Descriptor>;
    using Velocity = typename Base::Velocity;

    static constexpr int q = Descriptor::q;
    static constexpr int d = Descriptor::d;

    explicit BGKdynamics(T omega) : omega_(omega) {}

    T getOmega() const override { return omega_; }
    void setOmega(T omega) override { omega_ = omega; }

    /// Kinematic viscosity implied by a relaxation rate.
    static T kinematicViscosity(T omega) {
        return static_cast<T>(Descriptor::cs2) * (T(1) / omega - T(0.5));
    }

    /// Second-order equilibrium distribution (uSqr = u.u passed in).
    static T equilibrium(int i, T rho, const Velocity& u, T uSqr) {
        T ciu = T(0);
        for (int a = 0; a < d; ++a) ciu += static_cast<T>(Descriptor::c[i][a]) * u[a];
        const T invCs2 = static_cast<T>(Descriptor::invCs2);
        return static_cast<T>(Descriptor::t[i]) * rho *
               (T(1) + invCs2 * ciu + T(0.5) * invCs2 * invCs2 * ciu * ciu -
                T(0.5) * invCs2 * uSqr);
    }

    T computeEquilibrium(int iPop, T rho, const Velocity& u, T uSqr) const override {
        return equilibrium(iPop, rho, u, uSqr);
    }

    T computeDensity(const Cell<T, Descriptor>& cell) const override {
        T rho = T(0);
        for (int i = 0; i < q; ++i) rho += cell[i];
        return rho;
    }

    void computeVelocity(const Cell<T, Descriptor>& cell, Velocity& u) const override {
        T rho = T(0);
        Velocity j{};
        for (int i = 0; i < q; ++i) {
            const T fi = cell[i];
            rho += fi;
            for (int a = 0; a < d; ++a) j[a] += fi * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        for (int a = 0; a < d; ++a) u[a] = j[a] * invRho;
    }

    void collide(Cell<T, Descriptor>& cell) const override {
        T rho = T(0);
        Velocity j{};
        for (int i = 0; i < q; ++i) {
            const T fi = cell[i];
            rho += fi;
            for (int a = 0; a < d; ++a) j[a] += fi * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        Velocity u{};
        T uSqr = T(0);
        for (int a = 0; a < d; ++a) {
            u[a] = j[a] * invRho;
            uSqr += u[a] * u[a];
        }
        for (int i = 0; i < q; ++i) {
            const T feq = equilibrium(i, rho, u, uSqr);
            cell[i] -= omega_ * (cell[i] - feq);
        }
    }

private:
    T omega_;
};

}  // namespace cyberfluids
