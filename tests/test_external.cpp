/// Verifies external-field storage: a plain descriptor has zero external
/// scalars and null origins (no allocation, no null deref), while a Forced
/// variant round-trips per-cell external values through the lattice and Cell.

#include <cstdint>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/external.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::D2Q9;
using cyberfluids::descriptors::ForcedD2Q9;

int main() {
    // Plain D2Q9: no external fields, cells report none.
    {
        using Lattice = cyberfluids::BlockLattice2D<double, D2Q9>;
        Lattice lat(4, 3);
        CF_CHECK(lat.externalField().numScalars() == 0);
        auto cell = lat.get(2, 1);
        CF_CHECK(!cell.hasExternal());
    }

    // ForcedD2Q9: 2 external scalars per cell; round-trip via lattice and Cell.
    {
        using Lattice = cyberfluids::BlockLattice2D<double, ForcedD2Q9>;
        const std::int64_t nx = 4, ny = 3;
        Lattice lat(nx, ny);
        CF_CHECK(lat.externalField().numScalars() == 2);

        // Write via the lattice's external field.
        for (std::int64_t x = 0; x < nx; ++x)
            for (std::int64_t y = 0; y < ny; ++y) {
                const std::int64_t c = lat.index(x, y);
                lat.externalField()(0, c) = 10.0 * x + y;
                lat.externalField()(1, c) = -(10.0 * x + y);
            }

        // Read back via a Cell view.
        for (std::int64_t x = 0; x < nx; ++x)
            for (std::int64_t y = 0; y < ny; ++y) {
                auto cell = lat.get(x, y);
                CF_CHECK(cell.hasExternal());
                CF_CHECK(cell.external(0) == 10.0 * x + y);
                CF_CHECK(cell.external(1) == -(10.0 * x + y));
            }

        // Writing through the Cell reaches the lattice storage.
        auto cell = lat.get(1, 2);
        cell.external(0) = 99.0;
        CF_CHECK(lat.externalField()(0, lat.index(1, 2)) == 99.0);
    }

    if (cftest::failures == 0) std::printf("external: all checks passed\n");
    return cftest::failures;
}
