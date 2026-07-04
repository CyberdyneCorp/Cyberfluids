/// Verifies the NumPP-backed SoA population storage: allocation, zero-init,
/// round-trip read/write, and the contiguous {q, ncells} memory layout.
/// This test links NumPP and exercises the full dependency chain.

#include <cstdint>

#include <numpp/core/ndarray.hpp>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/populations.hpp"
#include "testing.hpp"

using cyberfluids::PopulationField;
using cyberfluids::descriptors::D2Q9;

int main() {
    using PF = PopulationField<double, D2Q9>;
    const std::int64_t ncells = 100;
    PF f(ncells);

    // Shape and size of the backing NumPP tensor.
    CF_CHECK(f.ncells() == ncells);
    CF_CHECK(PF::q == 9);
    CF_CHECK(f.array().size() == 9 * ncells);
    CF_CHECK(f.array().shape().size() == 2);
    CF_CHECK(f.array().shape()[0] == 9);
    CF_CHECK(f.array().shape()[1] == ncells);

    // numpp::zeros must zero-initialize.
    for (int i = 0; i < 9; ++i)
        for (std::int64_t c = 0; c < ncells; ++c)
            CF_CHECK(f(i, c) == 0.0);

    // Round-trip a distinctive pattern.
    for (int i = 0; i < 9; ++i)
        for (std::int64_t c = 0; c < ncells; ++c)
            f(i, c) = i * 1000.0 + static_cast<double>(c);
    for (int i = 0; i < 9; ++i)
        for (std::int64_t c = 0; c < ncells; ++c)
            CF_CHECK(f(i, c) == i * 1000.0 + static_cast<double>(c));

    // SoA layout: adjacent cells in one direction are contiguous; consecutive
    // directions are ncells apart.
    CF_CHECK(&f(0, 1) - &f(0, 0) == 1);
    CF_CHECK(&f(1, 0) - &f(0, 0) == ncells);

    if (cftest::failures == 0)
        std::printf("populations: all checks passed\n");
    return cftest::failures;
}
