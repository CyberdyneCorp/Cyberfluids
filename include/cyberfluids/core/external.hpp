#pragma once

#include <cstdint>
#include <type_traits>

#include <numpp/core/creation.hpp>
#include <numpp/core/dtype.hpp>
#include <numpp/core/ndarray.hpp>

#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/external_traits.hpp"

namespace cyberfluids {

/// Structure-of-Arrays store of per-cell external scalars: a NumPP tensor of
/// shape {nExt, ncells} sharing the populations' per-cell stride (ncells), so a
/// Cell addresses external scalars exactly as it addresses populations. It is a
/// separate allocation from PopulationField and is node-local — streaming never
/// moves it. When the descriptor declares no external fields (nExt == 0) it
/// allocates nothing and hands out null origins.
/// See openspec/specs/external-fields/spec.md.
template <class T, LatticeDescriptor Descriptor>
class ExternalField {
    static_assert(std::is_same_v<T, double>, "MVP: double only");
    static constexpr int nExt = numExternalScalars<Descriptor>();

public:
    explicit ExternalField(std::int64_t ncells) : ncells_(ncells) {
        if constexpr (nExt > 0)
            data_ = numpp::zeros({static_cast<std::int64_t>(nExt), ncells}, numpp::kFloat64);
    }

    static constexpr int numScalars() { return nExt; }
    std::int64_t ncells() const { return ncells_; }

    /// External scalar `s` of cell `c` (0 <= s < nExt).
    T& operator()(int s, std::int64_t c) {
        return data_.template typed_data<T>()[static_cast<std::int64_t>(s) * ncells_ + c];
    }
    const T& operator()(int s, std::int64_t c) const {
        return data_.template typed_data<T>()[static_cast<std::int64_t>(s) * ncells_ + c];
    }

    /// Pointer to external scalar 0 of cell `c` (stride ncells), or nullptr when
    /// there are no external fields (never dereferences an empty tensor).
    T* origin(std::int64_t c) {
        if constexpr (nExt > 0)
            return data_.template typed_data<T>() + c;
        else
            return nullptr;
    }

    numpp::ndarray& array() { return data_; }
    const numpp::ndarray& array() const { return data_; }

private:
    std::int64_t ncells_;
    numpp::ndarray data_;  // empty when nExt == 0
};

}  // namespace cyberfluids
