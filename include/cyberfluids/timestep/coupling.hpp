#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/core/external_traits.hpp"

namespace cyberfluids {

/// One-way fluid -> scalar coupling: write a fluid lattice's velocity into the
/// external velocity field of an advection-diffusion lattice of matching
/// geometry, so the scalar is advected by the fluid. Backend-dispatched, mirrors
/// the evolve.hpp seam. See openspec/specs/external-fields/spec.md.
template <class Backend = backend::Default, class FluidLattice, class ADLattice>
void copyVelocityToExternal(FluidLattice& fluid, ADLattice& ad) {
    using T = typename ADLattice::ValueType;
    using FluidDesc = typename FluidLattice::DescriptorType;
    using ADDesc = typename ADLattice::DescriptorType;
    static_assert(FluidLattice::dimension == ADLattice::dimension,
                  "fluid and AD lattices must share dimension");
    static_assert(numExternalScalars<ADDesc>() >= ADDesc::d,
                  "AD lattice needs a per-cell velocity external field (e.g. AdvectedD2Q5)");
    constexpr int d = ADDesc::d;
    constexpr int velOff = ADDesc::ExternalSpec::velocityBeginsAt;

    // Precondition: matching geometry (checked before the parallel loop so it
    // works under NDEBUG, unlike assert).
    bool sameGeometry = (fluid.ncells() == ad.ncells());
    for (int a = 0; a < d; ++a) sameGeometry = sameGeometry && (fluid.extent(a) == ad.extent(a));
    if (!sameGeometry)
        throw std::invalid_argument("copyVelocityToExternal: fluid and AD lattice extents differ");

    Backend::forEachIndex(ad.ncells(), [&](std::int64_t c) {
        auto fluidCell = fluid.cellByIndex(c);
        std::array<T, d> u{};
        fluid.getDynamics(c)->computeVelocity(fluidCell, u);
        for (int a = 0; a < d; ++a) ad.externalField()(velOff + a, c) = u[a];
    });
}

}  // namespace cyberfluids
