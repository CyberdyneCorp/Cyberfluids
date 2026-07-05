#pragma once

#include <array>

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/dynamics.hpp"
#include "cyberfluids/dynamics/bgk.hpp"

namespace cyberfluids {

namespace detail::mrt3d {

/// d'Humières (2002) D3Q19 orthogonal moment basis, evaluated on this project's
/// D3Q19 velocity ordering (descriptors::D3Q19::c). Moment order:
///   0 rho, 1 e, 2 eps, 3 jx, 4 qx, 5 jy, 6 qy, 7 jz, 8 qz,
///   9 3pxx, 10 3pixx, 11 pww, 12 piww, 13 pxy, 14 pyz, 15 pxz, 16 mx, 17 my, 18 mz.
/// The rows are mutually orthogonal, so M^-1 = M^T diag(1/norm). Because the
/// moment equilibria are taken as M*feq, relaxing every non-conserved moment at
/// omega reduces the operator to `f - omega(f - feq)` = BGK, exactly.
inline constexpr std::array<std::array<int, 19>, 19> M = {{
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
    {{-30, -11, -11, -11, -11, -11, -11, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8}},
    {{12, -4, -4, -4, -4, -4, -4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
    {{0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0}},
    {{0, -4, 4, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0}},
    {{0, 0, 0, 1, -1, 0, 0, 1, -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, -1}},
    {{0, 0, 0, -4, 4, 0, 0, 1, -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, -1}},
    {{0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, -1, 1, 1, -1, -1, 1}},
    {{0, 0, 0, 0, 0, -4, 4, 0, 0, 0, 0, 1, -1, -1, 1, 1, -1, -1, 1}},
    {{0, 2, 2, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, 1, 1, -2, -2, -2, -2}},
    {{0, -4, -4, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, -2, -2, -2, -2}},
    {{0, 0, 0, 1, 1, -1, -1, 1, 1, 1, 1, -1, -1, -1, -1, 0, 0, 0, 0}},
    {{0, 0, 0, -2, -2, 2, 2, 1, 1, 1, 1, -1, -1, -1, -1, 0, 0, 0, 0}},
    {{0, 0, 0, 0, 0, 0, 0, 1, 1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, -1, -1}},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, -1, -1, 0, 0, 0, 0}},
    {{0, 0, 0, 0, 0, 0, 0, 1, -1, 1, -1, -1, 1, -1, 1, 0, 0, 0, 0}},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, -1, 1, -1, 1, 1, -1}},
    {{0, 0, 0, 0, 0, 0, 0, -1, 1, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1}},
}};

/// Squared row norms of M (rows are orthogonal).
inline constexpr std::array<int, 19> norm = {19,   2394, 252, 10, 40, 10, 40,
                                             10,   40,   36,  72, 12, 24, 4,
                                             4,    4,    8,   8,  8};

/// Indices of the second-order deviatoric-stress moments, which relax at omega
/// to set the shear viscosity: 3pxx, pww, pxy, pyz, pxz.
}  // namespace detail::mrt3d

/// Multiple-relaxation-time (MRT) collision for D3Q19. Mirrors the D2Q9
/// MRTdynamics: map to moment space with M, relax each moment toward M*feq at
/// its own rate, map back with M^-1 = M^T diag(1/norm). Setting every free rate
/// to omega recovers BGK exactly. The five shear moments relax at omega and set
/// nu = cs2 (1/omega - 1/2). See openspec/specs/collision-dynamics/spec.md.
template <class T, LatticeDescriptor Descriptor>
class MRTdynamics3D final : public Dynamics<T, Descriptor> {
    static_assert(Descriptor::numPop == 19 && Descriptor::d == 3,
                  "MRTdynamics3D is implemented for D3Q19");

public:
    using Base = Dynamics<T, Descriptor>;
    using Velocity = typename Base::Velocity;
    static constexpr int q = 19;

    /// omega sets the shear viscosity; the free rates (energy se, energy-square
    /// seps, energy-flux sq, fourth-order spi, third-order sm) default to
    /// d'Humières stability values. Pass them all equal to omega to recover BGK.
    explicit MRTdynamics3D(T omega, T se = T(1.19), T seps = T(1.4), T sq = T(1.2),
                           T spi = T(1.4), T sm = T(1.98))
        : omega_(omega), se_(se), seps_(seps), sq_(sq), spi_(spi), sm_(sm) {}

    T getOmega() const override { return omega_; }
    void setOmega(T omega) override { omega_ = omega; }
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
    void computeVelocity(const Cell<T, Descriptor>& cell, Velocity& u) const override {
        T rho = T(0);
        Velocity jm{};
        for (int i = 0; i < q; ++i) {
            const T fi = cell[i];
            rho += fi;
            for (int a = 0; a < 3; ++a) jm[a] += fi * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        for (int a = 0; a < 3; ++a) u[a] = jm[a] * invRho;
    }

    void collide(Cell<T, Descriptor>& cell) const override {
        T rho = T(0);
        Velocity jm{};
        for (int i = 0; i < q; ++i) {
            const T fi = cell[i];
            rho += fi;
            for (int a = 0; a < 3; ++a) jm[a] += fi * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        Velocity u{jm[0] * invRho, jm[1] * invRho, jm[2] * invRho};
        const T uSqr = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];

        std::array<T, q> feq{};
        for (int i = 0; i < q; ++i)
            feq[i] = BGKdynamics<T, Descriptor>::equilibrium(i, rho, u, uSqr);

        const std::array<T, q> s = rateVector(omega_, se_, seps_, sq_, spi_, sm_);
        std::array<T, q> mRelaxed{};
        for (int k = 0; k < q; ++k) {
            T m = T(0), meq = T(0);
            for (int i = 0; i < q; ++i) {
                m += static_cast<T>(detail::mrt3d::M[k][i]) * cell[i];
                meq += static_cast<T>(detail::mrt3d::M[k][i]) * feq[i];
            }
            mRelaxed[k] = m - s[k] * (m - meq);
        }
        mapBack(cell, mRelaxed);
    }

    /// Relaxation vector in moment order. Conserved moments (rho, jx, jy, jz)
    /// have rate 0 (their m-meq is identically 0, so BGK-reduction is exact).
    static std::array<T, q> rateVector(T omega, T se, T seps, T sq, T spi, T sm) {
        return {T(0), se,    seps,  T(0), sq,    T(0), sq,   T(0), sq,   omega,
                spi,  omega, spi,   omega, omega, omega, sm,  sm,   sm};
    }
    /// f_i = sum_k M[k][i] mRelaxed[k] / norm[k].
    static void mapBack(Cell<T, Descriptor>& cell, const std::array<T, q>& mRelaxed) {
        for (int i = 0; i < q; ++i) {
            T fi = T(0);
            for (int k = 0; k < q; ++k)
                fi += static_cast<T>(detail::mrt3d::M[k][i]) * mRelaxed[k] /
                      static_cast<T>(detail::mrt3d::norm[k]);
            cell[i] = fi;
        }
    }

private:
    T omega_, se_, seps_, sq_, spi_, sm_;
};

/// D3Q19 MRT with a uniform body force via moment-space Guo forcing:
///   m* = m - s(m - meq) + (1 - s/2) M S,
/// where S_i is the Guo population source (without the scalar prefactor, which
/// is applied per-moment as (1 - s_k/2)). Reduces to ForcedBGKdynamics exactly
/// when every free rate equals omega. See openspec/specs/collision-dynamics/spec.md.
template <class T, LatticeDescriptor Descriptor>
class ForcedMRTdynamics3D final : public Dynamics<T, Descriptor> {
    static_assert(Descriptor::numPop == 19 && Descriptor::d == 3,
                  "ForcedMRTdynamics3D is implemented for D3Q19");

public:
    using Base = Dynamics<T, Descriptor>;
    using Velocity = typename Base::Velocity;
    static constexpr int q = 19;
    static constexpr int d = 3;

    ForcedMRTdynamics3D(T omega, const Velocity& force, T se = T(1.19), T seps = T(1.4),
                        T sq = T(1.2), T spi = T(1.4), T sm = T(1.98))
        : omega_(omega), se_(se), seps_(seps), sq_(sq), spi_(spi), sm_(sm), force_(force) {}

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
    /// Physical (Guo half-force) velocity u = (sum_i f_i c_i + F/2)/rho.
    void computeVelocity(const Cell<T, Descriptor>& cell, Velocity& u) const override {
        T rho = T(0);
        Velocity jm{};
        for (int i = 0; i < q; ++i) {
            const T fi = cell[i];
            rho += fi;
            for (int a = 0; a < d; ++a) jm[a] += fi * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        for (int a = 0; a < d; ++a) u[a] = (jm[a] + T(0.5) * force_[a]) * invRho;
    }

    void collide(Cell<T, Descriptor>& cell) const override {
        T rho = T(0);
        Velocity jm{};
        for (int i = 0; i < q; ++i) {
            const T fi = cell[i];
            rho += fi;
            for (int a = 0; a < d; ++a) jm[a] += fi * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        Velocity u{};
        T uSqr = T(0);
        for (int a = 0; a < d; ++a) {
            u[a] = (jm[a] + T(0.5) * force_[a]) * invRho;  // half-force velocity
            uSqr += u[a] * u[a];
        }

        std::array<T, q> feq{};
        std::array<T, q> src{};  // Guo source S_i WITHOUT the (1-s/2) prefactor
        const T invCs2 = static_cast<T>(Descriptor::invCs2);
        for (int i = 0; i < q; ++i) {
            feq[i] = BGKdynamics<T, Descriptor>::equilibrium(i, rho, u, uSqr);
            T ciu = T(0);
            for (int a = 0; a < d; ++a) ciu += static_cast<T>(Descriptor::c[i][a]) * u[a];
            T s = T(0);
            for (int a = 0; a < d; ++a) {
                const T ci = static_cast<T>(Descriptor::c[i][a]);
                s += ((ci - u[a]) * invCs2 + ciu * invCs2 * invCs2 * ci) * force_[a];
            }
            src[i] = static_cast<T>(Descriptor::t[i]) * s;
        }

        const std::array<T, q> s = MRTdynamics3D<T, Descriptor>::rateVector(omega_, se_, seps_,
                                                                            sq_, spi_, sm_);
        std::array<T, q> mRelaxed{};
        for (int k = 0; k < q; ++k) {
            T m = T(0), meq = T(0), ms = T(0);
            for (int i = 0; i < q; ++i) {
                const T mki = static_cast<T>(detail::mrt3d::M[k][i]);
                m += mki * cell[i];
                meq += mki * feq[i];
                ms += mki * src[i];  // (M S)[k]
            }
            mRelaxed[k] = m - s[k] * (m - meq) + (T(1) - T(0.5) * s[k]) * ms;
        }
        MRTdynamics3D<T, Descriptor>::mapBack(cell, mRelaxed);
    }

private:
    T omega_, se_, seps_, sq_, spi_, sm_;
    Velocity force_;
};

}  // namespace cyberfluids
