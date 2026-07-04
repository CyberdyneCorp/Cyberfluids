/// Verifies ScalarField and TensorField: allocation, zero-init, round-trip,
/// and C-order (row-major) memory layout, in 2D and 3D.

#include <cstdint>

#include <numpp/core/ndarray.hpp>

#include "cyberfluids/core/fields.hpp"
#include "testing.hpp"

using cyberfluids::ScalarField;
using cyberfluids::TensorField;

int main() {
    // ---- 2D scalar field (4 x 3) ----
    ScalarField<double, 2> rho(4, 3);
    CF_CHECK(rho.array().size() == 12);
    CF_CHECK(rho.extent(0) == 4 && rho.extent(1) == 3);
    for (std::int64_t x = 0; x < 4; ++x)
        for (std::int64_t y = 0; y < 3; ++y) CF_CHECK(rho(x, y) == 0.0);
    for (std::int64_t x = 0; x < 4; ++x)
        for (std::int64_t y = 0; y < 3; ++y) rho(x, y) = x * 10.0 + y;
    for (std::int64_t x = 0; x < 4; ++x)
        for (std::int64_t y = 0; y < 3; ++y) CF_CHECK(rho(x, y) == x * 10.0 + y);
    // C order: y is contiguous, x strides by ny.
    CF_CHECK(&rho(0, 1) - &rho(0, 0) == 1);
    CF_CHECK(&rho(1, 0) - &rho(0, 0) == 3);

    // ---- 2D velocity field (N=2) ----
    TensorField<double, 2, 2> u(4, 3);
    CF_CHECK(u.array().size() == 4 * 3 * 2);
    CF_CHECK((TensorField<double, 2, 2>::components) == 2);
    u(2, 1, 0) = 5.0;
    u(2, 1, 1) = 6.0;
    CF_CHECK(u(2, 1, 0) == 5.0);
    CF_CHECK(u(2, 1, 1) == 6.0);
    // Components are the fastest-varying axis.
    CF_CHECK(&u(2, 1, 1) - &u(2, 1, 0) == 1);

    // ---- 3D scalar field (2 x 2 x 2) ----
    ScalarField<double, 3> s(2, 2, 2);
    CF_CHECK(s.array().size() == 8);
    s(1, 1, 1) = 9.0;
    CF_CHECK(s(1, 1, 1) == 9.0);
    CF_CHECK(&s(0, 0, 1) - &s(0, 0, 0) == 1);
    CF_CHECK(&s(1, 0, 0) - &s(0, 0, 0) == 4);

    if (cftest::failures == 0) std::printf("fields: all checks passed\n");
    return cftest::failures;
}
