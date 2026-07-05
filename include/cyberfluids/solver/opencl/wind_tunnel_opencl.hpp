#pragma once

#if defined(CYBERFLUIDS_WITH_OPENCL)

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cyberfluids/geometry/voxel_field.hpp"

namespace cyberfluids::solver {

/// GPU wind tunnel (D3Q19) running real OpenCL device kernels — a bespoke
/// device-kernel solver (like the Metal cavity), NOT the CPU-only forEachIndex
/// seam. It mirrors the CPU solver::WindTunnel3D API and physics exactly (porous
/// convex-blend collide + far-field/outlet/interior stream), so the CPU solver
/// serves as its fp64 validation oracle. fp32 on the device. Populations stay
/// resident on the GPU across steps; the host copies back only for readout.
/// See docs/backends.md and openspec/specs/hardware-backends/spec.md.
class WindTunnelOpenCL {
public:
    WindTunnelOpenCL(std::int64_t nx, std::int64_t ny, std::int64_t nz, double omega, double uIn);
    ~WindTunnelOpenCL();
    WindTunnelOpenCL(const WindTunnelOpenCL&) = delete;
    WindTunnelOpenCL& operator=(const WindTunnelOpenCL&) = delete;

    /// Analytic sphere obstacle (centre + radius in lattice cells).
    void setObstacleSphere(double cx, double cy, double cz, double radius, bool sharp = true);
    /// Embed a voxelized object's solid fraction at offset (ox,oy,oz).
    void setObstacleField(const geometry::VoxelField& obj, int ox, int oy, int oz,
                          bool sharp = false);

    void run(std::int64_t steps);

    std::int64_t nx() const;
    std::int64_t ny() const;
    std::int64_t nz() const;

    /// Flux velocity (accumulated in fp64 on the host so the read-back adds no error).
    std::array<double, 3> velocity(std::int64_t x, std::int64_t y, std::int64_t z) const;
    double solidFraction(std::int64_t x, std::int64_t y, std::int64_t z) const;
    /// Raw {q, ncells} populations copied off the device (for a determinism check).
    std::vector<float> downloadPopulations() const;
    void writeVtk(const std::string& path) const;

    /// True if an OpenCL platform with a device is usable on this machine.
    static bool openclAvailable();

private:
    void setNs(std::int64_t x, std::int64_t y, std::int64_t z, double ns);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cyberfluids::solver

#endif  // CYBERFLUIDS_WITH_OPENCL
