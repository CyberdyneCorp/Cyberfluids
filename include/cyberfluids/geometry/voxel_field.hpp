#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace cyberfluids::geometry {

/// A dense axis-aligned signed-distance voxel grid in Cyberfluids' own cell
/// order (z-fastest: idx = (x*ny + y)*nz + z), so it drops straight onto a
/// BlockLattice without any transpose. `distance` is the signed distance from a
/// cell centre to the nearest solid surface, NEGATIVE inside the solid — the
/// same convention as CyberMeshGenerator's SignedDistance mode. Produced either
/// by the analytic voxelizer below (dependency-free, for tests/primitives) or by
/// the optional CyberMeshGenerator bridge (which re-indexes its x-fastest grid
/// into this order). See openspec/specs/geometry-and-io/spec.md.
struct VoxelField {
    int nx = 0, ny = 0, nz = 0;
    double spacing = 1.0;                 ///< cubic cell size (world units)
    std::array<double, 3> origin{0, 0, 0};  ///< world coord of the CENTRE of cell (0,0,0)
    std::vector<float> distance;          ///< signed distance, z-fastest, negative inside

    std::int64_t index(int x, int y, int z) const {
        return (static_cast<std::int64_t>(x) * ny + y) * nz + z;
    }
    std::int64_t cellCount() const {
        return static_cast<std::int64_t>(nx) * ny * nz;
    }
    /// Count of cells strictly inside the solid (distance < 0).
    std::int64_t solidCellCount() const {
        std::int64_t n = 0;
        for (float d : distance)
            if (d < 0.0f) ++n;
        return n;
    }
};

/// Re-index a grid given in X-FASTEST order (`src[(k*ny+j)*nx+i]` holds the value
/// at cell (i,j,k)) into a z-fastest VoxelField (`distance[(x*ny+y)*nz+z]`). The
/// physical axes coincide (i<->x, j<->y, k<->z); only the flattening differs.
/// This is the exact transform CyberMeshGenerator's grid needs — kept here,
/// CMG-free, so it is unit-testable without CMG. A naive linear copy is WRONG
/// whenever nx != nz.
inline VoxelField fromXFastest(int nx, int ny, int nz, double spacing,
                               std::array<double, 3> origin, const std::vector<float>& src) {
    VoxelField f;
    f.nx = nx;
    f.ny = ny;
    f.nz = nz;
    f.spacing = spacing;
    f.origin = origin;
    f.distance.resize(static_cast<std::size_t>(f.cellCount()));
    for (int x = 0; x < nx; ++x)
        for (int y = 0; y < ny; ++y)
            for (int z = 0; z < nz; ++z) {
                const std::size_t srcIdx =
                    (static_cast<std::size_t>(z) * ny + y) * nx + x;  // x-fastest source
                f.distance[static_cast<std::size_t>(f.index(x, y, z))] = src[srcIdx];
            }
    return f;
}

/// Map a signed distance (world units, negative inside) to a solid fraction
/// `ns in [0,1]` for partial bounce-back: a linear ramp of width `width` cells
/// centred on the surface. Deep inside -> 1, far outside -> 0, on the surface
/// (phi = 0) -> 0.5. Fed to PorousForcedBGKdynamics (ns=0 fluid, ns=1 solid).
inline double sdfToSolidFraction(double phi, double spacing, double width = 1.0) {
    const double t = 0.5 - phi / (width * spacing);
    return t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
}

// ---- Analytic implicit surfaces (signed distance, negative inside). ----------

struct Sphere {
    std::array<double, 3> center{0, 0, 0};
    double radius = 1.0;
    double sdf(double x, double y, double z) const {
        const double dx = x - center[0], dy = y - center[1], dz = z - center[2];
        return std::sqrt(dx * dx + dy * dy + dz * dz) - radius;
    }
};

/// Solid where coordinate `axis` (0=x,1=y,2=z) lies in [lo, hi].
struct Slab {
    int axis = 1;
    double lo = 0.0, hi = 1.0;
    double sdf(double x, double y, double z) const {
        const double p = (axis == 0) ? x : (axis == 1) ? y : z;
        return std::max(lo - p, p - hi);  // < 0 inside the slab, > 0 outside
    }
};

/// Sample an implicit surface at every cell centre into a VoxelField. Cell
/// (x,y,z) centre is `origin + (x,y,z)*spacing`.
template <class Sdf>
VoxelField voxelizeImplicit(int nx, int ny, int nz, double spacing,
                            std::array<double, 3> origin, const Sdf& surface) {
    VoxelField f;
    f.nx = nx;
    f.ny = ny;
    f.nz = nz;
    f.spacing = spacing;
    f.origin = origin;
    f.distance.resize(static_cast<std::size_t>(f.cellCount()));
    for (int x = 0; x < nx; ++x)
        for (int y = 0; y < ny; ++y)
            for (int z = 0; z < nz; ++z) {
                const double px = origin[0] + x * spacing;
                const double py = origin[1] + y * spacing;
                const double pz = origin[2] + z * spacing;
                f.distance[static_cast<std::size_t>(f.index(x, y, z))] =
                    static_cast<float>(surface.sdf(px, py, pz));
            }
    return f;
}

/// Write the solid fraction derived from `field` into a 3D lattice's ext::Scalar
/// slot (the ns consumed by PorousForcedBGKdynamics). The lattice extents must
/// match the field. `sharp` uses a hard 0/1 classification (phi<0 -> solid);
/// otherwise the SDF ramp is used. See dynamics/porous.hpp.
template <class Lattice>
void stampSolidFraction(Lattice& lattice, const VoxelField& field, bool sharp = false,
                        int nsOffset = 0, double width = 1.0) {
    // The field and lattice share the z-fastest index, so their cell counts must
    // match — otherwise field.index() would write past the external buffer (which
    // is unchecked). CMG derives grid dims from the mesh bbox, so the caller must
    // size the lattice to the grid; reject a mismatch rather than corrupt memory.
    if (field.cellCount() != lattice.ncells())
        throw std::invalid_argument(
            "stampSolidFraction: VoxelField cell count does not match lattice ncells");
    auto& ext = lattice.externalField();
    for (int x = 0; x < field.nx; ++x)
        for (int y = 0; y < field.ny; ++y)
            for (int z = 0; z < field.nz; ++z) {
                const double phi = field.distance[static_cast<std::size_t>(field.index(x, y, z))];
                const double ns = sharp ? (phi < 0.0 ? 1.0 : 0.0)
                                        : sdfToSolidFraction(phi, field.spacing, width);
                ext(nsOffset, field.index(x, y, z)) = ns;  // lattice uses the same z-fastest index
            }
}

}  // namespace cyberfluids::geometry
