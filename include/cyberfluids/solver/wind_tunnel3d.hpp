#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "cyberfluids/backend/backend.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/dynamics/porous.hpp"
#include "cyberfluids/geometry/voxel_field.hpp"
#include "cyberfluids/io/vtk_writer.hpp"
#include "cyberfluids/timestep/evolve.hpp"

namespace cyberfluids::solver {

/// 3D wind tunnel (external flow past an obstacle) on D3Q19. The obstacle is a
/// solid-fraction field `ns` consumed by PorousForcedBGKdynamics (force = 0), so
/// `ns=1` cells are no-slip (node bounce-back) with no extra boundary code. Flow
/// is driven by a far-field free-stream: the inlet (x=0) and the four transverse
/// walls (y and z extremes) are held at the equilibrium of `u=(Uin,0,0), rho=1`
/// every step (a stable velocity+density Dirichlet far field), and the outlet
/// (x=nx-1) is a zero-gradient copy of its upstream neighbour. Interior cells use
/// an ordinary pull-stream. Write the result to VTK for ParaView streamlines.
///
/// The obstacle is set either analytically (setObstacleSphere — no mesh
/// dependency) or from a voxelized STL/OBJ (setObstacleField, embedding the
/// object grid at an offset in the larger tunnel). See
/// openspec/specs/geometry-and-io and boundary-conditions specs.
template <class Backend = backend::Default, class T = double>
class WindTunnel3D {
    using D = descriptors::PorousD3Q19;
    using Base = descriptors::D3Q19;
    using Dyn = PorousForcedBGKdynamics<T, D>;
    static constexpr int nsOffset = Dyn::nsOffset;

public:
    WindTunnel3D(std::int64_t nx, std::int64_t ny, std::int64_t nz, T omega, T uIn)
        // Need a distinct inlet, interior, and outlet plane in x, and a fluid
        // interior between the transverse walls — so every extent must be >= 3.
        : nx_((requireMinExtent(nx, ny, nz), nx)),
          ny_(ny),
          nz_(nz),
          uIn_(uIn),
          lattice_(nx, ny, nz),
          scratch_(nx * ny * nz),
          dyn_(std::make_shared<Dyn>(omega, std::array<T, 3>{T(0), T(0), T(0)})) {
        lattice_.attributeDynamics(lattice_.getBoundingBox(), dyn_);
        initEquilibrium();
    }

    static T kinematicViscosity(T omega) { return Dyn::kinematicViscosity(omega); }
    /// omega for a target Reynolds number Re = Uin * Lchar / nu.
    static T omegaForReynolds(T uIn, T lChar, T re) {
        const T nu = uIn * lChar / re;
        return T(1) / (T(3) * nu + T(0.5));
    }

    void initEquilibrium() {
        const T uSqr = uIn_ * uIn_;
        for (std::int64_t c = 0; c < lattice_.ncells(); ++c)
            for (int i = 0; i < Base::q; ++i)
                lattice_.populations()(i, c) =
                    BGKdynamics<T, D>::equilibrium(i, T(1), {uIn_, T(0), T(0)}, uSqr);
    }

    /// An analytic sphere obstacle (no mesh dependency). Fills the ns field.
    void setObstacleSphere(T cx, T cy, T cz, T radius, bool sharp = true) {
        const geometry::Sphere s{{static_cast<double>(cx), static_cast<double>(cy),
                                  static_cast<double>(cz)},
                                 static_cast<double>(radius)};
        auto& ext = lattice_.externalField();
        for (std::int64_t x = 0; x < nx_; ++x)
            for (std::int64_t y = 0; y < ny_; ++y)
                for (std::int64_t z = 0; z < nz_; ++z) {
                    const double phi = s.sdf(x, y, z);
                    const double ns =
                        sharp ? (phi < 0.0 ? 1.0 : 0.0) : geometry::sdfToSolidFraction(phi, 1.0);
                    ext(nsOffset, index(x, y, z)) = ns;
                    if (ns > 0.5) seedRest(index(x, y, z));
                }
    }

    /// Embed a (typically smaller) voxelized object at offset (ox,oy,oz) in the
    /// tunnel, writing its solid fraction into the ns field. Cells outside the
    /// object keep their current ns (0 = fluid by default). Silently clips any
    /// part of the object that falls outside the tunnel.
    void setObstacleField(const geometry::VoxelField& obj, int ox, int oy, int oz,
                          bool sharp = false) {
        auto& ext = lattice_.externalField();
        for (int x = 0; x < obj.nx; ++x)
            for (int y = 0; y < obj.ny; ++y)
                for (int z = 0; z < obj.nz; ++z) {
                    const std::int64_t tx = ox + x, ty = oy + y, tz = oz + z;
                    if (tx < 0 || tx >= nx_ || ty < 0 || ty >= ny_ || tz < 0 || tz >= nz_)
                        continue;
                    const double phi = obj.distance[static_cast<std::size_t>(obj.index(x, y, z))];
                    const double ns = sharp ? (phi < 0.0 ? 1.0 : 0.0)
                                            : geometry::sdfToSolidFraction(phi, obj.spacing);
                    ext(nsOffset, index(tx, ty, tz)) = ns;
                    if (ns > 0.5) seedRest(index(tx, ty, tz));
                }
    }

    void step() {
        cyberfluids::collide<Backend>(lattice_);
        streamWithBoundaries();
        std::swap(lattice_.populations(), scratch_);
    }
    void run(std::int64_t steps) {
        for (std::int64_t s = 0; s < steps; ++s) step();
    }

    std::int64_t nx() const { return nx_; }
    std::int64_t ny() const { return ny_; }
    std::int64_t nz() const { return nz_; }

    std::array<T, 3> velocity(std::int64_t x, std::int64_t y, std::int64_t z) {
        auto cell = lattice_.get(x, y, z);
        std::array<T, 3> u{};
        dyn_->computeVelocity(cell, u);  // (j + F/2)/rho with F=0 -> j/rho (~0 in solid)
        return u;
    }
    T solidFraction(std::int64_t x, std::int64_t y, std::int64_t z) {
        return lattice_.externalField()(nsOffset, index(x, y, z));
    }

    /// Emit a legacy-VTK STRUCTURED_POINTS file (velocity vector, speed, and
    /// solid mask) for ParaView (Slice, Stream Tracer, Glyph).
    void writeVtk(const std::string& path) {
        io::VtkStructuredWriter<3> w(nx_, ny_, nz_);
        using Coord = io::VtkStructuredWriter<3>::Coord;
        w.addVector("velocity", [this](Coord c) { return velocity(c[0], c[1], c[2]); });
        w.addScalar("speed", [this](Coord c) {
            const auto u = velocity(c[0], c[1], c[2]);
            return std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
        });
        w.addScalar("solid", [this](Coord c) { return solidFraction(c[0], c[1], c[2]); });
        w.write(path);
    }

private:
    static void requireMinExtent(std::int64_t nx, std::int64_t ny, std::int64_t nz) {
        if (nx < 3 || ny < 3 || nz < 3)
            throw std::invalid_argument("WindTunnel3D: each extent must be >= 3");
    }
    std::int64_t index(std::int64_t x, std::int64_t y, std::int64_t z) const {
        return (x * ny_ + y) * nz_ + z;
    }

    /// Seed a solid cell at rest (rho=1, u=0): its symmetric equilibrium is
    /// invariant under bounce-back, so the solid stays quiescent rather than
    /// trapping the free-stream momentum it was initialized with.
    void seedRest(std::int64_t c) {
        for (int i = 0; i < Base::q; ++i)
            lattice_.populations()(i, c) =
                BGKdynamics<T, D>::equilibrium(i, T(1), {T(0), T(0), T(0)}, T(0));
    }

    void streamWithBoundaries() {
        auto& src = lattice_.populations();
        const std::int64_t nx = nx_, ny = ny_, nz = nz_;
        const T uSqr = uIn_ * uIn_;
        Backend::forEachIndex(lattice_.ncells(), [&, nx, ny, nz, uSqr](std::int64_t c) {
            const std::int64_t z = c % nz;
            const std::int64_t y = (c / nz) % ny;
            const std::int64_t x = c / (ny * nz);

            if (x == nx - 1) {  // outlet: zero-gradient (copy upstream plane)
                const std::int64_t up = ((nx - 2) * ny + y) * nz + z;
                for (int i = 0; i < Base::q; ++i) scratch_(i, c) = src(i, up);
                return;
            }
            if (x == 0 || y == 0 || y == ny - 1 || z == 0 || z == nz - 1) {
                // Inlet + transverse walls: free-stream equilibrium Dirichlet —
                // UNLESS the obstacle reaches this face, in which case the solid
                // cell keeps its (bounce-back) collision result so no-slip still
                // holds there rather than being overwritten with free-stream.
                if (lattice_.externalField()(nsOffset, c) > T(0.5)) {
                    for (int i = 0; i < Base::q; ++i) scratch_(i, c) = src(i, c);
                } else {
                    for (int i = 0; i < Base::q; ++i)
                        scratch_(i, c) =
                            BGKdynamics<T, D>::equilibrium(i, T(1), {uIn_, T(0), T(0)}, uSqr);
                }
                return;
            }
            // interior: ordinary pull-stream (all sources in range, no wrap)
            for (int i = 0; i < Base::q; ++i) {
                const std::int64_t sx = x - Base::c[i][0];
                const std::int64_t sy = y - Base::c[i][1];
                const std::int64_t sz = z - Base::c[i][2];
                scratch_(i, c) = src(i, (sx * ny + sy) * nz + sz);
            }
        });
    }

    std::int64_t nx_, ny_, nz_;
    T uIn_;
    BlockLattice3D<T, D> lattice_;
    PopulationField<T, D> scratch_;
    std::shared_ptr<Dyn> dyn_;
};

}  // namespace cyberfluids::solver
