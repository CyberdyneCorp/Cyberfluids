/// Guarded (CYBERFLUIDS_GEOMETRY=ON) round-trip through CyberMeshGenerator:
///   - a known unit-cube STL voxelizes to the correct volume (within a few %),
///     confirming the STL reader + voxelizer + x-fastest->z-fastest re-index;
///   - the Stanford bunny loads and voxelizes to a sane non-empty grid.
/// CF_CUBE_STL / CF_BUNNY_STL are provided by the CMake test registration.

#include <cmath>
#include <cstdio>

#include "cyberfluids/geometry/cmg_loader.hpp"
#include "testing.hpp"

using cyberfluids::geometry::loadAndVoxelize;
using cyberfluids::geometry::VoxelField;

int main() {
    // ---- Unit cube: voxel volume matches L^3 = 1. --------------------------
    {
        const VoxelField f = loadAndVoxelize(CF_CUBE_STL, 32, 1);
        CF_CHECK(f.nx > 0 && f.ny > 0 && f.nz > 0);
        CF_CHECK(static_cast<std::int64_t>(f.distance.size()) == f.cellCount());
        const double vol = static_cast<double>(f.solidCellCount()) * f.spacing * f.spacing * f.spacing;
        CF_CHECK(std::fabs(vol - 1.0) / 1.0 < 0.05);  // exact cube volume is 1
    }

    // ---- Asymmetric box (4x1x1): CMG derives asymmetric grid dims from the
    // bbox. (The x-fastest->z-fastest re-index correctness is covered by the
    // always-on fromXFastest unit test in test_voxelize_sphere, which a transpose
    // provably fails; here we just confirm the loader handles a non-cubic mesh.)
    {
        const VoxelField f = loadAndVoxelize(CF_XLONG_STL, 40, 1);
        CF_CHECK(f.solidCellCount() > 0);
        CF_CHECK(f.nx > 2 * f.nz);  // 4:1 x:z aspect ratio preserved in the grid
    }

    // ---- Stanford bunny: loads and produces a non-empty solid. -------------
    {
        const VoxelField f = loadAndVoxelize(CF_BUNNY_STL, 32, 2);
        CF_CHECK(f.nx > 0 && f.ny > 0 && f.nz > 0);
        CF_CHECK(static_cast<std::int64_t>(f.distance.size()) == f.cellCount());
        CF_CHECK(f.solidCellCount() > 0);  // watertight mesh has interior
    }

    if (cftest::failures == 0) std::printf("cmg_roundtrip: all checks passed\n");
    return cftest::failures;
}
