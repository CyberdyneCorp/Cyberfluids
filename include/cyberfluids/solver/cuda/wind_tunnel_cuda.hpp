#pragma once

#if defined(CYBERFLUIDS_WITH_CUDA)

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cyberfluids/geometry/voxel_field.hpp"

namespace cyberfluids::solver {

/// CUDA wind tunnel (D3Q19, fp32) — the CUDA mirror of WindTunnelOpenCL. The
/// device kernels (src/cuda/wind_tunnel_cuda.cu) are line-for-line translations
/// of the OpenCL kernels validated on Apple GPU (Linf/Uin ~ 1e-5 vs the fp64 CPU
/// oracle); this path is authored here but compiled/run with nvcc on an NVIDIA
/// machine. Same public API as WindTunnelOpenCL so the same test drives both.
/// See openspec/specs/hardware-backends/spec.md.
class WindTunnelCuda {
public:
    WindTunnelCuda(std::int64_t nx, std::int64_t ny, std::int64_t nz, double omega, double uIn);
    ~WindTunnelCuda();
    WindTunnelCuda(const WindTunnelCuda&) = delete;
    WindTunnelCuda& operator=(const WindTunnelCuda&) = delete;

    void setObstacleSphere(double cx, double cy, double cz, double radius, bool sharp = true);
    void setObstacleField(const geometry::VoxelField& obj, int ox, int oy, int oz,
                          bool sharp = false);
    void run(std::int64_t steps);

    std::int64_t nx() const;
    std::int64_t ny() const;
    std::int64_t nz() const;

    std::array<double, 3> velocity(std::int64_t x, std::int64_t y, std::int64_t z) const;
    double solidFraction(std::int64_t x, std::int64_t y, std::int64_t z) const;
    std::vector<float> downloadPopulations() const;
    void writeVtk(const std::string& path) const;

    /// True if a usable CUDA device is present.
    static bool cudaAvailable();

private:
    void setNs(std::int64_t x, std::int64_t y, std::int64_t z, double ns);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cyberfluids::solver

#endif  // CYBERFLUIDS_WITH_CUDA
