/// The ONLY translation unit that includes CyberMeshGenerator. It reads a mesh
/// and voxelizes it, then re-indexes CMG's x-fastest grid into Cyberfluids'
/// z-fastest VoxelField. Keeping CMG confined here means the core library and all
/// public headers stay CMG-free.

#include "cyberfluids/geometry/cmg_loader.hpp"

#include <string>

#include "cmg/io/io.hpp"
#include "cmg/voxelize/voxelize.hpp"

namespace cyberfluids::geometry {

VoxelField loadAndVoxelize(const std::string& path, int resolution, int pad) {
    auto plc = cmg::io::read_plc(path);
    if (!plc) throw GeometryError("read_plc('" + path + "'): " + plc.error().message);

    cmg::voxelize::VoxelOptions opts;
    opts.resolution = resolution;
    opts.pad = pad;
    opts.mode = cmg::voxelize::VoxelMode::SignedDistance;
    auto grid = cmg::voxelize::voxelize(*plc, opts);
    if (!grid) throw GeometryError("voxelize('" + path + "'): " + grid.error().message);

    // CMG's grid.distance is x-fastest ((k*ny+j)*nx+i); re-index into our
    // z-fastest VoxelField via the shared, unit-tested helper (never a memcpy).
    return fromXFastest(grid->nx, grid->ny, grid->nz, grid->spacing,
                        {grid->origin.x, grid->origin.y, grid->origin.z}, grid->distance);
}

}  // namespace cyberfluids::geometry
