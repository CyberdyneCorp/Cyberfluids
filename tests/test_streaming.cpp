/// Verifies streaming: single-population propagation along c_i (incl. diagonal
/// and periodic wrap), mass conservation, and that fused collideAndStream equals
/// separate collide + streamPeriodic.

#include <cstdint>
#include <memory>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/timestep/evolve.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::D2Q9;
using Lattice = cyberfluids::BlockLattice2D<double, D2Q9>;
using Pop = cyberfluids::PopulationField<double, D2Q9>;

namespace {
void fillPattern(Lattice& lat) {
    for (std::int64_t c = 0; c < lat.ncells(); ++c)
        for (int i = 0; i < 9; ++i)
            lat.populations()(i, c) = 0.1 + 0.001 * (i + 0.5 * c);
}
}  // namespace

int main() {
    // 1) Axis propagation: dir 1 = (1,0) at (1,1) -> (2,1).
    {
        Lattice lat(3, 3);
        Pop scratch(lat.ncells());
        lat.get(1, 1)[1] = 1.0;
        cyberfluids::streamPeriodic(lat, scratch);
        CF_CHECK(lat.populations()(1, lat.index(2, 1)) == 1.0);
        double s = 0.0;
        for (std::int64_t c = 0; c < lat.ncells(); ++c) s += lat.populations()(1, c);
        CF_CHECK_CLOSE(s, 1.0, 1e-15);
    }

    // 2) Periodic wrap: dir 1 at x=2 (nx=3) -> x=0.
    {
        Lattice lat(3, 3);
        Pop scratch(lat.ncells());
        lat.get(2, 1)[1] = 1.0;
        cyberfluids::streamPeriodic(lat, scratch);
        CF_CHECK(lat.populations()(1, lat.index(0, 1)) == 1.0);
    }

    // 3) Diagonal: dir 5 = (1,1) at (0,0) -> (1,1).
    {
        Lattice lat(3, 3);
        Pop scratch(lat.ncells());
        lat.get(0, 0)[5] = 1.0;
        cyberfluids::streamPeriodic(lat, scratch);
        CF_CHECK(lat.populations()(5, lat.index(1, 1)) == 1.0);
    }

    // 4) Fused collideAndStream == separate collide + streamPeriodic.
    {
        Lattice a(4, 4), b(4, 4);
        a.attributeDynamics(a.getBoundingBox(),
                            std::make_shared<cyberfluids::BGKdynamics<double, D2Q9>>(1.0 / 0.6));
        b.attributeDynamics(b.getBoundingBox(),
                            std::make_shared<cyberfluids::BGKdynamics<double, D2Q9>>(1.0 / 0.6));
        fillPattern(a);
        fillPattern(b);
        Pop sa(a.ncells()), sb(b.ncells());
        cyberfluids::collideAndStream(a, sa);
        cyberfluids::collide(b);
        cyberfluids::streamPeriodic(b, sb);
        for (std::int64_t c = 0; c < a.ncells(); ++c)
            for (int i = 0; i < 9; ++i)
                CF_CHECK_CLOSE(a.populations()(i, c), b.populations()(i, c), 1e-13);
    }

    if (cftest::failures == 0) std::printf("streaming: all checks passed\n");
    return cftest::failures;
}
