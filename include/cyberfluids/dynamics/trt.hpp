#pragma once

#include <array>

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/dynamics.hpp"
#include "cyberfluids/dynamics/bgk.hpp"

namespace cyberfluids {

/// Two-relaxation-time (TRT) collision. Populations are split into symmetric
/// (even) and antisymmetric (odd) parts relaxed at `omega_plus` and
/// `omega_minus`. `omega_plus` fixes the kinematic viscosity exactly as in BGK;
/// `omega_minus` is derived from a "magic" parameter
/// `Lambda = (1/omega_plus - 1/2)(1/omega_minus - 1/2)` (default 1/4), which
/// controls stability and wall placement. With `omega_minus == omega_plus` TRT
/// reduces to BGK. See openspec/specs/collision-dynamics/spec.md.
template <class T, LatticeDescriptor Descriptor>
class TRTdynamics final : public Dynamics<T, Descriptor> {
public:
    using Base = Dynamics<T, Descriptor>;
    using Velocity = typename Base::Velocity;
    static constexpr int q = Descriptor::q;
    static constexpr int d = Descriptor::d;

    explicit TRTdynamics(T omegaPlus, T magic = T(0.25))
        : omegaPlus_(omegaPlus), magic_(magic), omegaMinus_(minusFromMagic(omegaPlus, magic)) {}

    T getOmega() const override { return omegaPlus_; }
    void setOmega(T omega) override {
        omegaPlus_ = omega;
        omegaMinus_ = minusFromMagic(omegaPlus_, magic_);
    }
    T getOmegaMinus() const { return omegaMinus_; }

    static T kinematicViscosity(T omegaPlus) {
        return BGKdynamics<T, Descriptor>::kinematicViscosity(omegaPlus);
    }

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

        std::array<T, q> feq{};
        for (int i = 0; i < q; ++i)
            feq[i] = BGKdynamics<T, Descriptor>::equilibrium(i, rho, u, uSqr);

        std::array<T, q> f{};
        for (int i = 0; i < q; ++i) f[i] = cell[i];

        for (int i = 0; i < q; ++i) {
            const int io = Descriptor::opposite[i];
            const T fPlus = T(0.5) * (f[i] + f[io]);
            const T fMinus = T(0.5) * (f[i] - f[io]);
            const T eqPlus = T(0.5) * (feq[i] + feq[io]);
            const T eqMinus = T(0.5) * (feq[i] - feq[io]);
            cell[i] = f[i] - omegaPlus_ * (fPlus - eqPlus) - omegaMinus_ * (fMinus - eqMinus);
        }
    }

private:
    static T minusFromMagic(T omegaPlus, T magic) {
        const T tauPlusMinusHalf = T(1) / omegaPlus - T(0.5);
        return T(1) / (magic / tauPlusMinusHalf + T(0.5));
    }

    T omegaPlus_;
    T magic_;
    T omegaMinus_;
};

}  // namespace cyberfluids
