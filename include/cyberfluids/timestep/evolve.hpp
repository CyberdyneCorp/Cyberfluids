#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"

namespace cyberfluids {

/// Collision step: apply each cell's attached dynamics in place. Backend-
/// dispatched; the loop body is identical across backends.
template <class Backend = backend::Default, class T, LatticeDescriptor Descriptor, int Dim>
void collide(BlockLattice<T, Descriptor, Dim>& lattice) {
    Backend::forEachIndex(lattice.ncells(), [&lattice](std::int64_t c) {
        auto* dyn = lattice.getDynamics(c);
        if (dyn != nullptr) {
            auto cell = lattice.cellByIndex(c);
            dyn->collide(cell);
        }
    });
}

namespace detail {
inline std::int64_t wrap(std::int64_t v, std::int64_t n) { return ((v % n) + n) % n; }
}  // namespace detail

/// Streaming step with full periodic wrap-around on every axis. Pulls each
/// post-collision population `f_i(x - c_i)` into `x`, writing into `scratch`,
/// then swaps `scratch` with the lattice's populations. Backend-dispatched.
template <class Backend = backend::Default, class T, LatticeDescriptor Descriptor, int Dim>
void streamPeriodic(BlockLattice<T, Descriptor, Dim>& lattice,
                    PopulationField<T, Descriptor>& scratch) {
    constexpr int q = Descriptor::q;
    const std::int64_t nx = lattice.extent(0);
    const std::int64_t ny = lattice.extent(1);
    const std::int64_t nz = (Dim == 3) ? lattice.extent(2) : 1;
    auto& src = lattice.populations();

    Backend::forEachIndex(lattice.ncells(), [&, nx, ny, nz](std::int64_t c) {
        std::int64_t x, y, z;
        if constexpr (Dim == 2) {
            y = c % ny;
            x = c / ny;
            z = 0;
        } else {
            z = c % nz;
            y = (c / nz) % ny;
            x = c / (ny * nz);
        }
        for (int i = 0; i < q; ++i) {
            const std::int64_t sx = detail::wrap(x - Descriptor::c[i][0], nx);
            const std::int64_t sy = detail::wrap(y - Descriptor::c[i][1], ny);
            std::int64_t sc;
            if constexpr (Dim == 2) {
                sc = sx * ny + sy;
            } else {
                const std::int64_t sz = detail::wrap(z - Descriptor::c[i][2], nz);
                sc = (sx * ny + sy) * nz + sz;
            }
            scratch(i, c) = src(i, sc);
        }
    });

    std::swap(src, scratch);
}

/// Fused collide-and-stream (periodic). Equivalent to `collide` followed by
/// `streamPeriodic`.
template <class Backend = backend::Default, class T, LatticeDescriptor Descriptor, int Dim>
void collideAndStream(BlockLattice<T, Descriptor, Dim>& lattice,
                      PopulationField<T, Descriptor>& scratch) {
    collide<Backend>(lattice);
    streamPeriodic<Backend>(lattice, scratch);
}

}  // namespace cyberfluids
