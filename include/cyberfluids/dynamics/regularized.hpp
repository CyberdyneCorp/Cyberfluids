#pragma once

#include <array>

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/dynamics.hpp"
#include "cyberfluids/dynamics/bgk.hpp"

namespace cyberfluids {

/// Regularized BGK collision. Before relaxing, the non-equilibrium part is
/// projected onto the second-order (stress) Hermite basis, discarding ghost
/// modes: f_i = feq_i + (1 - omega) f_i^reg, where f_i^reg is rebuilt from the
/// non-equilibrium momentum flux Pi_ab. Improves stability at low viscosity.
/// See openspec/specs/collision-dynamics/spec.md.
template <class T, LatticeDescriptor Descriptor>
class RegularizedBGKdynamics final : public Dynamics<T, Descriptor> {
public:
    using Base = Dynamics<T, Descriptor>;
    using Velocity = typename Base::Velocity;
    static constexpr int q = Descriptor::q;
    static constexpr int d = Descriptor::d;

    explicit RegularizedBGKdynamics(T omega) : omega_(omega) {}

    T getOmega() const override { return omega_; }
    void setOmega(T omega) override { omega_ = omega; }

    T computeEquilibrium(int iPop, T rho, const Velocity& u, T uSqr) const override {
        return BGKdynamics<T, Descriptor>::equilibrium(iPop, rho, u, uSqr);
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

        // Non-equilibrium momentum flux Pi_ab = sum_i c_ia c_ib (f_i - feq_i).
        T Pi[d][d];
        for (int a = 0; a < d; ++a)
            for (int b = 0; b < d; ++b) Pi[a][b] = T(0);
        for (int i = 0; i < q; ++i) {
            const T fneq = cell[i] - BGKdynamics<T, Descriptor>::equilibrium(i, rho, u, uSqr);
            for (int a = 0; a < d; ++a)
                for (int b = 0; b < d; ++b)
                    Pi[a][b] += static_cast<T>(Descriptor::c[i][a]) *
                                static_cast<T>(Descriptor::c[i][b]) * fneq;
        }

        const T cs2 = static_cast<T>(Descriptor::cs2);
        const T invCs2 = static_cast<T>(Descriptor::invCs2);
        for (int i = 0; i < q; ++i) {
            // Q_iab : Pi_ab  with Q_iab = c_ia c_ib - cs2 delta_ab.
            T qContract = T(0);
            for (int a = 0; a < d; ++a)
                for (int b = 0; b < d; ++b) {
                    const T Q = static_cast<T>(Descriptor::c[i][a]) *
                                    static_cast<T>(Descriptor::c[i][b]) -
                                (a == b ? cs2 : T(0));
                    qContract += Q * Pi[a][b];
                }
            const T fRegNeq =
                static_cast<T>(Descriptor::t[i]) * T(0.5) * invCs2 * invCs2 * qContract;
            const T feq = BGKdynamics<T, Descriptor>::equilibrium(i, rho, u, uSqr);
            cell[i] = feq + (T(1) - omega_) * fRegNeq;
        }
    }

private:
    T omega_;
};

}  // namespace cyberfluids
