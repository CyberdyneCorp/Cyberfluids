#pragma once

#include <stdexcept>
#include <string>

#include "cyberfluids/geometry/voxel_field.hpp"

/// Optional STL/OBJ geometry import, backed by CyberMeshGenerator. This header is
/// CMG-FREE — only src/geometry/cmg_loader.cpp includes CMG. Available when the
/// build is configured with -DCYBERFLUIDS_GEOMETRY=ON. See
/// openspec/specs/geometry-and-io/spec.md.
namespace cyberfluids::geometry {

/// Thrown when a mesh cannot be read or voxelized (wraps CMG's MeshError).
class GeometryError : public std::runtime_error {
public:
    explicit GeometryError(const std::string& what) : std::runtime_error(what) {}
};

/// Read a surface mesh (STL/OBJ/OFF/PLY/…, dispatched by extension) and voxelize
/// it into a signed-distance VoxelField in Cyberfluids' own z-fastest cell order
/// (negative inside the solid). The grid dimensions/origin/spacing are derived by
/// CMG from the mesh bounding box expanded by `pad` cells, with `resolution`
/// cubic cells along the longest axis — read `nx/ny/nz` back from the result.
/// The returned field can be stamped onto a matching lattice with
/// stampSolidFraction(). Throws GeometryError on failure.
VoxelField loadAndVoxelize(const std::string& path, int resolution, int pad = 1);

}  // namespace cyberfluids::geometry
