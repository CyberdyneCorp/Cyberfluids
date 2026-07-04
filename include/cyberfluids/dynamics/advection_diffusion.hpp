#pragma once

#include <array>

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/dynamics.hpp"
#include "cyberfluids/core/external_traits.hpp"

namespace cyberfluids {

/// Advection-diffusion (AD) BGK collision for the reduced stencils (D2Q5/D3Q7).
/// Transports a scalar `phi = sum_i f_i` at diffusivity `D = cs2 (1/omega - 1/2)`
/// using the FIRST-ORDER equilibrium `feq_i = t_i phi (1 + invCs2 (c_i . u))`.
/// The advection velocity `u` is read from the cell's external field (e.g. set
/// by coupling from a fluid lattice), NOT from the population first moment. The
/// descriptor must declare at least `d` external velocity scalars (e.g.
/// descriptors::AdvectedD2Q5). See openspec/specs/collision-dynamics/spec.md.
template <class T, LatticeDescriptor Descriptor>
class AdvectionDiffusionBGKdynamics final : public Dynamics<T, Descriptor> {
    static_assert(numVelocityScalars<Descriptor>() >= Descriptor::d,
                  "AdvectionDiffusionBGKdynamics needs a descriptor declaring an advection-"
                  "velocity external field with >= d scalars (e.g. descriptors::AdvectedD2Q5)");
    static_assert(numExternalScalars<Descriptor>() >=
                      Descriptor::ExternalSpec::velocityBeginsAt + Descriptor::d,
                  "velocity external block must fit within the declared external scalars");

public:
    using Base = Dynamics<T, Descriptor>;
    using Velocity = typename Base::Velocity;
    static constexpr int q = Descriptor::q;
    static constexpr int d = Descriptor::d;
    static constexpr int velOffset = Descriptor::ExternalSpec::velocityBeginsAt;

    explicit AdvectionDiffusionBGKdynamics(T omega) : omega_(omega) {}

    T getOmega() const override { return omega_; }
    void setOmega(T omega) override { omega_ = omega; }
    bool isAdvectionDiffusion() const override { return true; }

    /// Scalar diffusivity implied by the relaxation rate.
    static T diffusivity(T omega) {
        return static_cast<T>(Descriptor::cs2) * (T(1) / omega - T(0.5));
    }

    /// First-order advection-diffusion equilibrium.
    static T equilibrium(int i, T phi, const Velocity& u) {
        T ciu = T(0);
        for (int a = 0; a < d; ++a) ciu += static_cast<T>(Descriptor::c[i][a]) * u[a];
        return static_cast<T>(Descriptor::t[i]) * phi *
               (T(1) + static_cast<T>(Descriptor::invCs2) * ciu);
    }

    T computeEquilibrium(int iPop, T phi, const Velocity& u, T /*uSqr*/) const override {
        return equilibrium(iPop, phi, u);
    }

    /// The transported scalar phi = sum_i f_i.
    T computeDensity(const Cell<T, Descriptor>& cell) const override {
        T phi = T(0);
        for (int i = 0; i < q; ++i) phi += cell[i];
        return phi;
    }

    /// The advection velocity comes from the external field, not the populations.
    void computeVelocity(const Cell<T, Descriptor>& cell, Velocity& u) const override {
        for (int a = 0; a < d; ++a) u[a] = cell.external(velOffset + a);
    }

    void collide(Cell<T, Descriptor>& cell) const override {
        T phi = T(0);
        for (int i = 0; i < q; ++i) phi += cell[i];
        Velocity u{};
        for (int a = 0; a < d; ++a) u[a] = cell.external(velOffset + a);
        for (int i = 0; i < q; ++i) {
            const T feq = equilibrium(i, phi, u);
            cell[i] -= omega_ * (cell[i] - feq);
        }
    }

private:
    T omega_;
};

}  // namespace cyberfluids
