#pragma once

#include <array>

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptor.hpp"

namespace cyberfluids {

/// Abstract collision model attached to lattice cells. Defines the local
/// collision operator, the equilibrium distribution, and how macroscopic
/// moments are recovered from populations. Concrete models (BGK, TRT, MRT, …)
/// derive from this. Relaxation convention: `omega = 1 / tau`.
/// See openspec/specs/collision-dynamics/spec.md.
template <class T, LatticeDescriptor Descriptor>
class Dynamics {
public:
    using Velocity = std::array<T, Descriptor::d>;

    virtual ~Dynamics() = default;

    /// Relax the cell's populations toward equilibrium at rate `omega`.
    virtual void collide(Cell<T, Descriptor>& cell) const = 0;

    /// Equilibrium distribution f_iPop^eq for given density and velocity
    /// (uSqr = u·u passed in to avoid recomputation).
    virtual T computeEquilibrium(int iPop, T rho, const Velocity& u, T uSqr) const = 0;

    /// Zeroth moment: density rho = sum_i f_i.
    virtual T computeDensity(const Cell<T, Descriptor>& cell) const = 0;

    /// First moment: velocity u = (1/rho) sum_i f_i c_i.
    virtual void computeVelocity(const Cell<T, Descriptor>& cell, Velocity& u) const = 0;

    virtual T getOmega() const = 0;
    virtual void setOmega(T omega) = 0;

    virtual bool isBoundary() const { return false; }
    virtual bool isAdvectionDiffusion() const { return false; }
};

}  // namespace cyberfluids
