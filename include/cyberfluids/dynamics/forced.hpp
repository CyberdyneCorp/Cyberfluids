#pragma once

#include <array>

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/dynamics.hpp"
#include "cyberfluids/core/external_traits.hpp"
#include "cyberfluids/dynamics/bgk.hpp"

namespace cyberfluids {

/// BGK collision with a uniform (constant) body force via the Guo forcing
/// scheme. The velocity carries the half-force correction
/// `u = (sum_i f_i c_i + F/2)/rho`, and a source term is added after relaxation:
/// `S_i = (1 - omega/2) w_i [ (c_i - u)/cs2 + (c_i.u)/cs2^2 c_i ] . F`.
/// Per-cell (spatially varying) forcing is a future extension.
/// See openspec/specs/collision-dynamics/spec.md.
template <class T, LatticeDescriptor Descriptor>
class ForcedBGKdynamics final : public Dynamics<T, Descriptor> {
public:
    using Base = Dynamics<T, Descriptor>;
    using Velocity = typename Base::Velocity;
    static constexpr int q = Descriptor::q;
    static constexpr int d = Descriptor::d;

    ForcedBGKdynamics(T omega, const Velocity& force) : omega_(omega), force_(force) {}

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
    /// Velocity including the half-force correction (the physical fluid velocity).
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
            u[a] = (j[a] + T(0.5) * force_[a]) * invRho;  // Guo half-force velocity
            uSqr += u[a] * u[a];
        }

        const T invCs2 = static_cast<T>(Descriptor::invCs2);
        const T prefactor = T(1) - T(0.5) * omega_;
        for (int i = 0; i < q; ++i) {
            const T feq = BGKdynamics<T, Descriptor>::equilibrium(i, rho, u, uSqr);
            // Guo source term S_i.
            T ciu = T(0);
            for (int a = 0; a < d; ++a) ciu += static_cast<T>(Descriptor::c[i][a]) * u[a];
            T source = T(0);
            for (int a = 0; a < d; ++a) {
                const T ci = static_cast<T>(Descriptor::c[i][a]);
                source += ((ci - u[a]) * invCs2 + ciu * invCs2 * invCs2 * ci) * force_[a];
            }
            source *= prefactor * static_cast<T>(Descriptor::t[i]);
            cell[i] += -omega_ * (cell[i] - feq) + source;
        }
    }

private:
    T omega_;
    Velocity force_;
};

/// BGK collision with a PER-CELL (spatially varying) body force read from the
/// cell's external field, using the same Guo scheme as ForcedBGKdynamics. The
/// descriptor must declare at least `d` external scalars for the force (e.g.
/// descriptors::ForcedD2Q9). The host writes the force into the lattice's
/// external field before each collide. See openspec/specs/collision-dynamics/spec.md.
template <class T, LatticeDescriptor Descriptor>
class ExternalForceBGKdynamics final : public Dynamics<T, Descriptor> {
    static_assert(numForceScalars<Descriptor>() >= Descriptor::d,
                  "ExternalForceBGKdynamics needs a descriptor declaring a force external field "
                  "with >= d scalars (e.g. descriptors::ForcedD2Q9)");
    static_assert(numExternalScalars<Descriptor>() >=
                      Descriptor::ExternalSpec::forceBeginsAt + Descriptor::d,
                  "force external block must fit within the declared external scalars");

public:
    using Base = Dynamics<T, Descriptor>;
    using Velocity = typename Base::Velocity;
    static constexpr int q = Descriptor::q;
    static constexpr int d = Descriptor::d;
    static constexpr int forceOffset = Descriptor::ExternalSpec::forceBeginsAt;

    explicit ExternalForceBGKdynamics(T omega) : omega_(omega) {}

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
        Velocity force{};
        readForce(cell, force);
        T rho = T(0);
        Velocity j{};
        for (int i = 0; i < q; ++i) {
            const T fi = cell[i];
            rho += fi;
            for (int a = 0; a < d; ++a) j[a] += fi * static_cast<T>(Descriptor::c[i][a]);
        }
        const T invRho = T(1) / rho;
        for (int a = 0; a < d; ++a) u[a] = (j[a] + T(0.5) * force[a]) * invRho;
    }

    void collide(Cell<T, Descriptor>& cell) const override {
        Velocity force{};
        readForce(cell, force);
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
            u[a] = (j[a] + T(0.5) * force[a]) * invRho;
            uSqr += u[a] * u[a];
        }

        const T invCs2 = static_cast<T>(Descriptor::invCs2);
        const T prefactor = T(1) - T(0.5) * omega_;
        for (int i = 0; i < q; ++i) {
            const T feq = BGKdynamics<T, Descriptor>::equilibrium(i, rho, u, uSqr);
            T ciu = T(0);
            for (int a = 0; a < d; ++a) ciu += static_cast<T>(Descriptor::c[i][a]) * u[a];
            T source = T(0);
            for (int a = 0; a < d; ++a) {
                const T ci = static_cast<T>(Descriptor::c[i][a]);
                source += ((ci - u[a]) * invCs2 + ciu * invCs2 * invCs2 * ci) * force[a];
            }
            source *= prefactor * static_cast<T>(Descriptor::t[i]);
            cell[i] += -omega_ * (cell[i] - feq) + source;
        }
    }

private:
    void readForce(const Cell<T, Descriptor>& cell, Velocity& force) const {
        for (int a = 0; a < d; ++a) force[a] = cell.external(forceOffset + a);
    }
    T omega_;
};

}  // namespace cyberfluids
