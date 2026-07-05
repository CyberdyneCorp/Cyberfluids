/// Primary geometry oracle: walls defined purely by a solid-fraction field
/// (ns=1 slabs) must reproduce Poiseuille flow. A fully-periodic PorousD2Q9 box
/// is driven by a Guo body force; solid slabs at the y-extremes merge across the
/// periodic-y wrap into one block, leaving a single channel. Because ns=1 is
/// bit-exact node bounce-back (test_porous T2), this is "geometry-configured
/// walls == a hand-coded bounce-back channel", validated against the analytic
/// parabola and the hand-coded PoiseuilleChannel2D. Uses no CMG.

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/geometry/voxel_field.hpp"
#include "cyberfluids/solver/poiseuille_channel.hpp"
#include "cyberfluids/solver/porous_flow_2d.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::PorousD2Q9;

int main() {
    const double omega = 1.0, Fx = 1e-5;
    const double nu = 1.0 / 6.0;  // (1/omega - 1/2)/3
    const std::int64_t nx = 8, ts = 2, Nf = 48, nyTot = Nf + 2 * ts;  // 52

    // Fully-periodic forced box; walls come from a voxelized solid-fraction field
    // stamped through the SHIPPED geometry bridge (stampSolidFraction), not a
    // hand-coded BC. A 2D lattice is a VoxelField with nz=1 (index x*ny+y). Using
    // nx != ny here also guards the stamp against an x/y index transpose — a
    // transposed stamp would put walls in x and destroy the channel.
    cyberfluids::solver::PorousBox2D<> box(nx, nyTot, omega, Fx);
    cyberfluids::geometry::VoxelField slabs;
    slabs.nx = static_cast<int>(nx);
    slabs.ny = static_cast<int>(nyTot);
    slabs.nz = 1;
    slabs.spacing = 1.0;
    slabs.distance.resize(static_cast<std::size_t>(slabs.cellCount()));
    for (std::int64_t x = 0; x < nx; ++x)
        for (std::int64_t y = 0; y < nyTot; ++y)
            slabs.distance[static_cast<std::size_t>(slabs.index(static_cast<int>(x),
                                                                static_cast<int>(y), 0))] =
                (y < ts || y >= nyTot - ts) ? -1.0f : 1.0f;  // negative = solid slab
    cyberfluids::geometry::stampSolidFraction(box.lattice(), slabs, /*sharp=*/true);
    box.run(30000);

    // Wall planes sit at the fluid/solid mid-links: y = ts-0.5 and y = nyTot-ts-0.5.
    // Local channel coordinate Y = y - (ts - 0.5); channel height H = Nf.
    const double H = static_cast<double>(Nf);
    const double uMax = Fx / (2.0 * nu) * (H / 2) * (H / 2);  // 1.728e-2

    auto& pop = box.lattice().populations();
    auto rowUx = [&](std::int64_t y) {
        const std::int64_t c = 0 * nyTot + y;  // column x=0
        double rho = 0, jx = 0;
        for (int i = 0; i < 9; ++i) {
            const double fi = pop(i, c);
            rho += fi;
            jx += fi * PorousD2Q9::c[i][0];
        }
        return jx / rho;
    };

    // Hand-coded halfway-bounce-back channel of the same fluid width, for a
    // second, independent reference.
    cyberfluids::solver::PoiseuilleChannel2D<> ref(nx, Nf, omega, Fx);
    ref.run(30000);

    double linfAna = 0.0, linfRef = 0.0, vMax = 0.0;
    for (std::int64_t y = ts; y < nyTot - ts; ++y) {
        const double Y = static_cast<double>(y) - (ts - 0.5);
        const double uAna = Fx / (2.0 * nu) * Y * (H - Y);
        const double u = rowUx(y);
        linfAna = std::max(linfAna, std::fabs(u - uAna));

        const double uRef = ref.velocity(0, y - ts)[0];  // matching fluid row
        linfRef = std::max(linfRef, std::fabs(u - uRef));

        // Cross-flow (y-velocity) must stay ~0.
        const std::int64_t c = 0 * nyTot + y;
        double rho = 0, jy = 0;
        for (int i = 0; i < 9; ++i) {
            rho += pop(i, c);
            jy += pop(i, c) * PorousD2Q9::c[i][1];
        }
        vMax = std::max(vMax, std::fabs(jy / rho));
    }

    CF_CHECK(linfAna / uMax < 0.03);  // matches the analytic parabola
    CF_CHECK(linfRef / uMax < 0.03);  // matches the hand-coded channel
    CF_CHECK(vMax < 1e-6 * uMax + 1e-12);  // no cross-flow

    if (cftest::failures == 0) std::printf("geometry_poiseuille: all checks passed\n");
    return cftest::failures;
}
