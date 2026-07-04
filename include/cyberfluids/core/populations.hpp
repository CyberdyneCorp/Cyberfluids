#pragma once

#include <cstdint>
#include <type_traits>

#include <numpp/core/creation.hpp>
#include <numpp/core/dtype.hpp>
#include <numpp/core/ndarray.hpp>

#include "cyberfluids/core/descriptor.hpp"

namespace cyberfluids {

/// Structure-of-Arrays storage for the particle distribution functions
/// (populations) of a lattice, backed by a NumPP tensor of shape
/// `{q, ncells}` (component-major). Each direction's data is contiguous across
/// cells, which favours CPU vectorization and GPU coalescing.
/// See openspec/specs/core-data-structures/spec.md and numpp-scipp-foundation.
template <class T, LatticeDescriptor Descriptor>
class PopulationField {
    static_assert(std::is_same_v<T, double>,
                  "PopulationField currently supports T=double (MVP).");

public:
    static constexpr int q = Descriptor::q;

    explicit PopulationField(std::int64_t ncells)
        : ncells_(ncells),
          data_(numpp::zeros({static_cast<std::int64_t>(q), ncells},
                             numpp::kFloat64)) {}

    std::int64_t ncells() const { return ncells_; }

    /// Distribution function f_i at cell `c` (0 <= iPop < q, 0 <= c < ncells).
    T& operator()(int iPop, std::int64_t c) {
        return data_.typed_data<T>()[static_cast<std::int64_t>(iPop) * ncells_ + c];
    }
    const T& operator()(int iPop, std::int64_t c) const {
        return data_.typed_data<T>()[static_cast<std::int64_t>(iPop) * ncells_ + c];
    }

    /// Access the backing NumPP tensor (shape {q, ncells}).
    numpp::ndarray& array() { return data_; }
    const numpp::ndarray& array() const { return data_; }

private:
    std::int64_t ncells_;
    numpp::ndarray data_;
};

}  // namespace cyberfluids
