#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/core/external_traits.hpp"

namespace cyberfluids {

/// Boussinesq buoyancy parameters (lattice units).
template <class T>
struct BoussinesqParameters {
    T gravity = T(0);              // g magnitude along the buoyancy axis
    T thermalExpansion = T(1);     // beta
    T referenceTemperature = T(0); // T_ref
    int gravityAxis = 1;           // buoyancy axis (y in 2D, z in 3D)
    bool useLocalDensity = true;   // rho from fluid dynamics; else rho0
    T rho0 = T(1);
};

/// Two-way thermal feedback: write the buoyancy body force
/// `F = rho * g * beta * (T - T_ref)` (along the gravity axis; other components
/// zero) into the fluid lattice's per-cell external FORCE field, where
/// ExternalForceBGKdynamics reads it. `T` is the AD scalar at the matching cell
/// (`computeDensity`). Backend-dispatched; mirrors copyVelocityToExternal
/// (geometry precondition, unassigned-cell skip). The fluid descriptor must
/// declare a force external field (e.g. descriptors::ForcedD2Q9).
/// See openspec/specs/external-fields/spec.md.
template <class Backend = backend::Default, class FluidLattice, class ADLattice>
void applyBuoyancy(FluidLattice& fluid, ADLattice& temperature,
                   const BoussinesqParameters<typename FluidLattice::ValueType>& p) {
    using T = typename FluidLattice::ValueType;
    using FluidDesc = typename FluidLattice::DescriptorType;
    static_assert(FluidLattice::dimension == ADLattice::dimension,
                  "fluid and temperature lattices must share dimension");
    static_assert(numForceScalars<FluidDesc>() >= FluidDesc::d,
                  "applyBuoyancy needs a fluid descriptor with a force external field "
                  "(e.g. descriptors::ForcedD2Q9)");
    constexpr int d = FluidDesc::d;
    constexpr int forceOff = FluidDesc::ExternalSpec::forceBeginsAt;

    bool same = (fluid.ncells() == temperature.ncells());
    for (int a = 0; a < d; ++a) same = same && (fluid.extent(a) == temperature.extent(a));
    if (!same) throw std::invalid_argument("applyBuoyancy: fluid and temperature extents differ");

    const int axis = p.gravityAxis;
    Backend::forEachIndex(fluid.ncells(), [&](std::int64_t c) {
        T phi = T(0);
        if (auto* tdyn = temperature.getDynamics(c)) {
            auto tcell = temperature.cellByIndex(c);
            phi = tdyn->computeDensity(tcell);
        }
        T rho = p.rho0;
        if (p.useLocalDensity) {
            if (auto* fdyn = fluid.getDynamics(c)) {
                auto fcell = fluid.cellByIndex(c);
                rho = fdyn->computeDensity(fcell);
            }
        }
        const T force = rho * p.gravity * p.thermalExpansion * (phi - p.referenceTemperature);
        for (int a = 0; a < d; ++a)
            fluid.externalField()(forceOff + a, c) = (a == axis) ? force : T(0);
    });
}

}  // namespace cyberfluids
