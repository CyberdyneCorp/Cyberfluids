#pragma once

#include <cstdint>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/core/external_traits.hpp"
#include "cyberfluids/physics/shan_chen_eos.hpp"

namespace cyberfluids {

/// Single-component Shan-Chen interaction parameters (lattice units).
template <class T>
struct ShanChenParameters {
    T G = T(-6);     // interaction strength; separation requires |G| > 4/rho0
    T rho0 = T(1);   // pseudopotential reference density
};

/// Compute the density field rho[c] = sum_i f_i(c) into a caller-owned buffer of
/// length ncells (unassigned cells -> 0, mirroring collide()).
template <class Backend = backend::Default, class FluidLattice>
void computeDensityField(FluidLattice& fluid, typename FluidLattice::ValueType* rho) {
    using T = typename FluidLattice::ValueType;
    constexpr int q = FluidLattice::DescriptorType::q;
    auto& pop = fluid.populations();
    Backend::forEachIndex(fluid.ncells(), [&](std::int64_t c) {
        if (fluid.getDynamics(c) == nullptr) {
            rho[c] = T(0);
            return;
        }
        T s = T(0);
        for (int i = 0; i < q; ++i) s += pop(i, c);
        rho[c] = s;
    });
}

/// Non-local Shan-Chen interaction force (2D, periodic wrap), written into the
/// fluid's per-cell external force slots (consumed by ExternalForceBGKdynamics):
///   F(x) = -G psi(rho(x)) * sum_{i>0} w_i psi(rho(x+c_i)) c_i,  w_i = t_i.
/// `rho` is the density field from computeDensityField.
/// See openspec/specs/physical-models/spec.md.
template <class Backend = backend::Default, class FluidLattice>
void applyShanChenForce(FluidLattice& fluid, const typename FluidLattice::ValueType* rho,
                        const ShanChenParameters<typename FluidLattice::ValueType>& p) {
    using T = typename FluidLattice::ValueType;
    using D = typename FluidLattice::DescriptorType;
    static_assert(FluidLattice::dimension == 2, "Shan-Chen MVP is 2D");
    static_assert(numForceScalars<D>() >= D::d,
                  "applyShanChenForce needs a force external field (e.g. descriptors::ForcedD2Q9)");
    constexpr int q = D::q;
    constexpr int forceOff = D::ExternalSpec::forceBeginsAt;
    const std::int64_t nx = fluid.extent(0), ny = fluid.extent(1);
    const T rho0 = p.rho0, G = p.G;

    Backend::forEachIndex(fluid.ncells(), [&, nx, ny](std::int64_t c) {
        const std::int64_t y = c % ny, x = c / ny;
        const T psiSelf = shanChenPsi(rho[c], rho0);
        T sx = T(0), sy = T(0);
        for (int i = 1; i < q; ++i) {  // skip rest link (c_0 = 0)
            const std::int64_t xn = ((x + D::c[i][0]) % nx + nx) % nx;
            const std::int64_t yn = ((y + D::c[i][1]) % ny + ny) % ny;
            const T w = static_cast<T>(D::t[i]);
            const T psiN = shanChenPsi(rho[xn * ny + yn], rho0);
            sx += w * psiN * static_cast<T>(D::c[i][0]);
            sy += w * psiN * static_cast<T>(D::c[i][1]);
        }
        fluid.externalField()(forceOff + 0, c) = -G * psiSelf * sx;
        fluid.externalField()(forceOff + 1, c) = -G * psiSelf * sy;
    });
}

}  // namespace cyberfluids
