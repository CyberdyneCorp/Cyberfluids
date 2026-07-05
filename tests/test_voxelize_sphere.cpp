/// Validates the analytic voxelizer (dependency-free, no CMG):
///   - the inside-cell count times cell volume converges to the exact sphere
///     volume (4/3)pi R^3 as resolution rises (first-order midpoint rule — noisy
///     at coarse N, so we assert per-resolution bounds and a 4x-refinement gain,
///     not adjacent-step monotonicity);
///   - the SDF->solid-fraction ramp is 1 deep inside, 0 far outside, ~0.5 on
///     the surface.

#include <array>
#include <cmath>
#include <cstdio>

#include <vector>

#include "cyberfluids/geometry/voxel_field.hpp"
#include "testing.hpp"

using cyberfluids::geometry::fromXFastest;
using cyberfluids::geometry::Sphere;
using cyberfluids::geometry::sdfToSolidFraction;
using cyberfluids::geometry::voxelizeImplicit;

namespace {
// Relative volume error at N cells per axis over a cube of side L centred on the
// sphere (R = 1), voxelizing the signed distance and counting cells inside.
double volumeError(int N) {
    const double L = 3.0, R = 1.0, spacing = L / N;
    // Cell (0,0,0) centre sits half a cell inside the [-L/2, L/2] box.
    const std::array<double, 3> origin{-L / 2 + spacing / 2, -L / 2 + spacing / 2,
                                       -L / 2 + spacing / 2};
    const auto f = voxelizeImplicit(N, N, N, spacing, origin, Sphere{{0, 0, 0}, R});
    const double vol = static_cast<double>(f.solidCellCount()) * spacing * spacing * spacing;
    const double exact = 4.0 / 3.0 * M_PI * R * R * R;
    return std::fabs(vol - exact) / exact;
}
}  // namespace

int main() {
    // ---- Volume convergence. ----------------------------------------------
    const double e24 = volumeError(24), e48 = volumeError(48), e96 = volumeError(96);
    CF_CHECK(e96 < 0.03);
    CF_CHECK(e48 < 0.06);
    CF_CHECK(e24 < 0.12);
    // Midpoint volume error is non-monotone step-to-step; only assert a clear
    // gain across a 4x refinement (robust), not adjacent-step monotonicity.
    CF_CHECK(e96 < e24);

    // ---- SDF -> solid-fraction ramp. --------------------------------------
    const double h = 1.0;  // spacing
    CF_CHECK_CLOSE(sdfToSolidFraction(-h, h), 1.0, 1e-12);   // one cell inside -> solid
    CF_CHECK_CLOSE(sdfToSolidFraction(0.0, h), 0.5, 1e-12);  // on the surface
    CF_CHECK_CLOSE(sdfToSolidFraction(+h, h), 0.0, 1e-12);   // one cell outside -> fluid
    CF_CHECK(sdfToSolidFraction(-5 * h, h) == 1.0);          // clamped
    CF_CHECK(sdfToSolidFraction(+5 * h, h) == 0.0);

    // ---- X-fastest -> z-fastest re-index (the CMG bridge transform). -------
    // Asymmetric dims (nx != nz) with a distinct value per cell, so a naive
    // linear/bulk copy (the transpose bug) is provably caught: value(i,j,k) =
    // 100i + 10j + k must land at our cell (x=i, y=j, z=k).
    {
        const int nx = 4, ny = 2, nz = 3;
        std::vector<float> src(static_cast<std::size_t>(nx) * ny * nz);
        for (int i = 0; i < nx; ++i)
            for (int j = 0; j < ny; ++j)
                for (int k = 0; k < nz; ++k)
                    src[(static_cast<std::size_t>(k) * ny + j) * nx + i] =
                        static_cast<float>(100 * i + 10 * j + k);
        const auto f = fromXFastest(nx, ny, nz, 1.0, {0, 0, 0}, src);
        CF_CHECK(f.nx == nx && f.ny == ny && f.nz == nz);
        bool ok = true;
        for (int x = 0; x < nx; ++x)
            for (int y = 0; y < ny; ++y)
                for (int z = 0; z < nz; ++z)
                    if (f.distance[static_cast<std::size_t>(f.index(x, y, z))] !=
                        static_cast<float>(100 * x + 10 * y + z))
                        ok = false;
        CF_CHECK(ok);
    }

    if (cftest::failures == 0) std::printf("voxelize_sphere: all checks passed\n");
    return cftest::failures;
}
