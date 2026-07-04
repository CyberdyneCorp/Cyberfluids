#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

#include <numpp/core/ndarray.hpp>
#include <numpp/io/npy.hpp>

#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/populations.hpp"

namespace cyberfluids::io {

/// Serialize the populations {q, ncells} to a NumPy .npy file (byte-exact:
/// raw little-endian float64, no formatting). The populations are the entire
/// live lattice state between steps — the stream scratch buffer is transient and
/// the collision models are stateless — so this alone gives a faithful restart.
/// See openspec/specs/geometry-and-io/spec.md.
template <class T, LatticeDescriptor Descriptor>
void saveCheckpoint(const std::string& path, const PopulationField<T, Descriptor>& pop) {
    numpp::save(path, pop.array());
}

/// Restore populations previously written by saveCheckpoint. Throws on a shape
/// mismatch (wrong q or ncells).
template <class T, LatticeDescriptor Descriptor>
void loadCheckpoint(const std::string& path, PopulationField<T, Descriptor>& pop) {
    numpp::ndarray a = numpp::load(path);
    if (a.ndim() != 2 || a.shape()[0] != Descriptor::q || a.shape()[1] != pop.ncells())
        throw std::runtime_error("loadCheckpoint: population shape mismatch");
    const std::int64_t count = static_cast<std::int64_t>(Descriptor::q) * pop.ncells();
    std::memcpy(pop.array().template typed_data<T>(), a.template typed_data<T>(),
                static_cast<std::size_t>(count) * sizeof(T));
}

}  // namespace cyberfluids::io
