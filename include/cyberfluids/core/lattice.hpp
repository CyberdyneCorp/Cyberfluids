#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/dynamics.hpp"
#include "cyberfluids/core/geometry.hpp"
#include "cyberfluids/core/populations.hpp"

namespace cyberfluids {

/// A block lattice: an owning grid of LBM state (populations + per-cell
/// dynamics) in 2D or 3D. Cells are cheap views obtained via `get(...)`.
/// The lattice owns its storage (NumPP-backed populations) and its dynamics
/// (shared_ptr registry); nothing requires a manual delete.
/// See openspec/specs/core-data-structures/spec.md.
template <class T, LatticeDescriptor Descriptor, int Dim = Descriptor::d>
class BlockLattice {
    static_assert(Dim == Descriptor::d, "Lattice dimension must match Descriptor::d");

public:
    using CellType = Cell<T, Descriptor>;
    using DynamicsType = Dynamics<T, Descriptor>;

    BlockLattice(std::int64_t nx, std::int64_t ny) requires(Dim == 2)
        : n_{nx, ny},
          ncells_(nx * ny),
          pop_(nx * ny),
          cellDynamics_(static_cast<std::size_t>(nx * ny), nullptr) {}

    BlockLattice(std::int64_t nx, std::int64_t ny, std::int64_t nz) requires(Dim == 3)
        : n_{nx, ny, nz},
          ncells_(nx * ny * nz),
          pop_(nx * ny * nz),
          cellDynamics_(static_cast<std::size_t>(nx * ny * nz), nullptr) {}

    std::int64_t extent(int axis) const { return n_[axis]; }
    std::int64_t ncells() const { return ncells_; }

    Box<Dim> getBoundingBox() const {
        Box<Dim> b;
        for (int a = 0; a < Dim; ++a) {
            b.lo[a] = 0;
            b.hi[a] = n_[a] - 1;
        }
        return b;
    }

    std::int64_t index(std::int64_t x, std::int64_t y) const requires(Dim == 2) {
        return x * n_[1] + y;
    }
    std::int64_t index(std::int64_t x, std::int64_t y, std::int64_t z) const
        requires(Dim == 3) {
        return (x * n_[1] + y) * n_[2] + z;
    }

    CellType get(std::int64_t x, std::int64_t y) requires(Dim == 2) {
        return cellAt(index(x, y));
    }
    CellType get(std::int64_t x, std::int64_t y, std::int64_t z) requires(Dim == 3) {
        return cellAt(index(x, y, z));
    }

    PopulationField<T, Descriptor>& populations() { return pop_; }
    const PopulationField<T, Descriptor>& populations() const { return pop_; }

    /// Build a Cell view from a linear cell index (0 <= c < ncells).
    CellType cellByIndex(std::int64_t c) { return cellAt(c); }

    /// Assign a collision model to every cell in `box`. The lattice keeps the
    /// shared_ptr alive; cells hold a non-owning pointer to it.
    void attributeDynamics(const Box<Dim>& box, std::shared_ptr<DynamicsType> dynamics) {
        DynamicsType* raw = dynamics.get();
        registry_.push_back(std::move(dynamics));
        forEachInBox(box, [&](std::int64_t c) {
            cellDynamics_[static_cast<std::size_t>(c)] = raw;
        });
    }

    DynamicsType* getDynamics(std::int64_t c) const {
        return cellDynamics_[static_cast<std::size_t>(c)];
    }

private:
    CellType cellAt(std::int64_t c) {
        return CellType(&pop_(0, c), ncells_, cellDynamics_[static_cast<std::size_t>(c)]);
    }

    template <class F>
    void forEachInBox(const Box<Dim>& box, F&& f) {
        if constexpr (Dim == 2) {
            for (std::int64_t x = box.lo[0]; x <= box.hi[0]; ++x)
                for (std::int64_t y = box.lo[1]; y <= box.hi[1]; ++y)
                    f(index(x, y));
        } else {
            for (std::int64_t x = box.lo[0]; x <= box.hi[0]; ++x)
                for (std::int64_t y = box.lo[1]; y <= box.hi[1]; ++y)
                    for (std::int64_t z = box.lo[2]; z <= box.hi[2]; ++z)
                        f(index(x, y, z));
        }
    }

    std::array<std::int64_t, Dim> n_;
    std::int64_t ncells_;
    PopulationField<T, Descriptor> pop_;
    std::vector<std::shared_ptr<DynamicsType>> registry_;
    std::vector<DynamicsType*> cellDynamics_;
};

template <class T, LatticeDescriptor Descriptor>
using BlockLattice2D = BlockLattice<T, Descriptor, 2>;

template <class T, LatticeDescriptor Descriptor>
using BlockLattice3D = BlockLattice<T, Descriptor, 3>;

}  // namespace cyberfluids
