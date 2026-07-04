#pragma once

#include <array>
#include <type_traits>

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/dynamics.hpp"
#include "cyberfluids/dynamics/bgk.hpp"

namespace cyberfluids {

/// Multiple-relaxation-time (MRT) collision for D2Q9. Populations are mapped to
/// moment space with the Lallemand-Luo matrix M, each moment relaxes toward its
/// equilibrium at its own rate, then the result is mapped back with M^-1.
///
/// The moment equilibria are taken as the moments of the BGK equilibrium
/// (m_eq = M feq), which guarantees MRT reduces exactly to BGK when every
/// relaxation rate equals omega. The shear moments (pxx, pxy) relax at omega to
/// fix the viscosity; conserved moments (rho, jx, jy) are not relaxed; the
/// remaining rates default to standard stability values.
/// See openspec/specs/collision-dynamics/spec.md.
template <class T, LatticeDescriptor Descriptor>
class MRTdynamics final : public Dynamics<T, Descriptor> {
    static_assert(Descriptor::numPop == 9 && Descriptor::d == 2,
                  "MRTdynamics is implemented for D2Q9 in M1");

public:
    using Base = Dynamics<T, Descriptor>;
    using Velocity = typename Base::Velocity;
    static constexpr int q = 9;

    // Moment order: rho, e, eps, jx, qx, jy, qy, pxx, pxy.
    static constexpr std::array<std::array<int, 9>, 9> M = {{
        {{1, 1, 1, 1, 1, 1, 1, 1, 1}},
        {{-4, -1, -1, -1, -1, 2, 2, 2, 2}},
        {{4, -2, -2, -2, -2, 1, 1, 1, 1}},
        {{0, 1, 0, -1, 0, 1, -1, -1, 1}},
        {{0, -2, 0, 2, 0, 1, -1, -1, 1}},
        {{0, 0, 1, 0, -1, 1, 1, -1, -1}},
        {{0, 0, -2, 0, 2, 1, 1, -1, -1}},
        {{0, 1, -1, 1, -1, 0, 0, 0, 0}},
        {{0, 0, 0, 0, 0, 1, -1, 1, -1}},
    }};
    // Squared row norms (rows of M are orthogonal) -> M^-1 = M^T diag(1/norm).
    static constexpr std::array<T, 9> norm = {9, 36, 36, 6, 12, 6, 12, 4, 4};

    /// omega sets the viscosity (shear moments pxx, pxy). The three free rates
    /// (e, eps, q) default to common stability values; pass them equal to omega
    /// to recover BGK exactly.
    explicit MRTdynamics(T omega, T sE = T(1.63), T sEps = T(1.14), T sQ = T(1.9))
        : omega_(omega), sE_(sE), sEps_(sEps), sQ_(sQ) {}

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
            for (int a = 0; a < 2; ++a) jm[a] += fi * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        for (int a = 0; a < 2; ++a) u[a] = jm[a] * invRho;
    }

    void collide(Cell<T, Descriptor>& cell) const override {
        // Macroscopic moments for the BGK equilibrium.
        T rho = T(0);
        Velocity jm{};
        for (int i = 0; i < q; ++i) {
            const T fi = cell[i];
            rho += fi;
            for (int a = 0; a < 2; ++a) jm[a] += fi * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        Velocity u{jm[0] * invRho, jm[1] * invRho};
        const T uSqr = u[0] * u[0] + u[1] * u[1];

        std::array<T, 9> feq{};
        for (int i = 0; i < q; ++i)
            feq[i] = BGKdynamics<T, Descriptor>::equilibrium(i, rho, u, uSqr);

        // Transform to moment space: m = M f, meq = M feq.
        const std::array<T, 9> s = {T(0), sE_, sEps_, T(0), sQ_, T(0), sQ_, omega_, omega_};
        std::array<T, 9> mRelaxed{};
        for (int k = 0; k < q; ++k) {
            T m = T(0), meq = T(0);
            for (int i = 0; i < q; ++i) {
                m += static_cast<T>(M[k][i]) * cell[i];
                meq += static_cast<T>(M[k][i]) * feq[i];
            }
            mRelaxed[k] = m - s[k] * (m - meq);
        }

        // Map back: f_i = sum_k M[k][i] mRelaxed[k] / norm[k].
        for (int i = 0; i < q; ++i) {
            T fi = T(0);
            for (int k = 0; k < q; ++k)
                fi += static_cast<T>(M[k][i]) * mRelaxed[k] / norm[k];
            cell[i] = fi;
        }
    }

private:
    T omega_, sE_, sEps_, sQ_;
};

}  // namespace cyberfluids
