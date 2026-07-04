#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <numpp/core/ndarray.hpp>
#include <numpp/io/npy.hpp>

#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"

namespace cyberfluids::io {

namespace detail {
template <class T>
void copyContiguous(const numpp::ndarray& src, T* dst, std::int64_t count) {
    // A foreign .npy may be Fortran-ordered; typed_data() ignores strides, so a
    // raw memcpy of non-C-contiguous data would transpose the layout. Reject it.
    if (!src.c_contiguous())
        throw std::runtime_error("loadCheckpoint: checkpoint array is not C-contiguous");
    std::memcpy(dst, src.template typed_data<T>(), static_cast<std::size_t>(count) * sizeof(T));
}
}  // namespace detail

/// Serialize populations to a NumPy .npy. This PRIMITIVE validates only the cell
/// count on load (a PopulationField has no geometry) — prefer the BlockLattice
/// overloads below, which validate the full grid shape.
template <class T, LatticeDescriptor Descriptor>
void saveCheckpoint(const std::string& path, const PopulationField<T, Descriptor>& pop) {
    numpp::save(path, pop.array());
}
template <class T, LatticeDescriptor Descriptor>
void loadCheckpoint(const std::string& path, PopulationField<T, Descriptor>& pop) {
    numpp::ndarray a = numpp::load(path);
    if (a.ndim() != 2 || a.shape()[0] != Descriptor::q || a.shape()[1] != pop.ncells())
        throw std::runtime_error("loadCheckpoint: population cell-count mismatch");
    detail::copyContiguous(a, pop.array().template typed_data<T>(),
                           static_cast<std::int64_t>(Descriptor::q) * pop.ncells());
}

/// Serialize a lattice's populations to a NumPy .npy, reshaped to
/// {q, nx, ny[, nz]} so the file encodes the full geometry. The bytes are
/// identical to the {q, ncells} layout (row-major, contiguous).
template <class T, LatticeDescriptor Descriptor, int Dim>
void saveCheckpoint(const std::string& path, const BlockLattice<T, Descriptor, Dim>& lattice) {
    std::vector<std::int64_t> shape;
    shape.push_back(Descriptor::q);
    for (int a = 0; a < Dim; ++a) shape.push_back(lattice.extent(a));
    numpp::save(path, lattice.populations().array().reshape(shape));
}

/// Restore a lattice's populations, validating the full grid shape (q and every
/// extent) — so a same-ncells but different-shape lattice is rejected.
template <class T, LatticeDescriptor Descriptor, int Dim>
void loadCheckpoint(const std::string& path, BlockLattice<T, Descriptor, Dim>& lattice) {
    numpp::ndarray a = numpp::load(path);
    bool ok = (a.ndim() == Dim + 1) && (a.shape()[0] == Descriptor::q);
    for (int d = 0; d < Dim && ok; ++d) ok = (a.shape()[d + 1] == lattice.extent(d));
    if (!ok) throw std::runtime_error("loadCheckpoint: lattice grid shape mismatch");
    detail::copyContiguous(a, lattice.populations().array().template typed_data<T>(),
                           static_cast<std::int64_t>(Descriptor::q) * lattice.ncells());
}

}  // namespace cyberfluids::io
