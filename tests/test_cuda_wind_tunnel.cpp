/// CUDA analog of test_opencl_wind_tunnel: validates the CUDA wind tunnel (fp32)
/// against the CPU solver::WindTunnel3D (fp64) oracle. Skips cleanly (exit 0) when
/// no CUDA device is present. Runs on an NVIDIA machine with -DCYBERFLUIDS_CUDA=ON.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "cyberfluids/solver/cuda/wind_tunnel_cuda.hpp"
#include "cyberfluids/solver/wind_tunnel3d.hpp"
#include "testing.hpp"

using cyberfluids::solver::WindTunnel3D;
using cyberfluids::solver::WindTunnelCuda;

int main() {
    if (!WindTunnelCuda::cudaAvailable()) {
        std::printf("SKIP cuda_wind_tunnel: no CUDA device\n");
        return 0;
    }

    const std::int64_t nx = 48, ny = 24, nz = 24;
    const double Uin = 0.05, R = 4.0, cx = 14, cy = 12, cz = 12;
    const double omega = WindTunnel3D<>::omegaForReynolds(Uin, 2 * R, 20.0);
    const std::int64_t steps = 5000;

    WindTunnel3D<> cpu(nx, ny, nz, omega, Uin);
    cpu.setObstacleSphere(cx, cy, cz, R, true);
    cpu.run(steps);

    WindTunnelCuda gpu(nx, ny, nz, omega, Uin);
    gpu.setObstacleSphere(cx, cy, cz, R, true);
    gpu.run(steps);

    double linf = 0.0, sumDiff2 = 0.0, sumRef2 = 0.0, sumAbs = 0.0, solidMax = 0.0;
    std::int64_t fluidN = 0;
    for (std::int64_t x = 0; x < nx; ++x)
        for (std::int64_t y = 0; y < ny; ++y)
            for (std::int64_t z = 0; z < nz; ++z) {
                const auto ug = gpu.velocity(x, y, z);
                if (cpu.solidFraction(x, y, z) > 0.5) {
                    solidMax = std::max(solidMax, std::sqrt(ug[0] * ug[0] + ug[1] * ug[1] +
                                                            ug[2] * ug[2]));
                    continue;
                }
                const auto uc = cpu.velocity(x, y, z);
                double d2 = 0, r2 = 0, dmax = 0;
                for (int k = 0; k < 3; ++k) {
                    const double d = ug[k] - uc[k];
                    d2 += d * d;
                    r2 += uc[k] * uc[k];
                    dmax = std::max(dmax, std::fabs(d));
                }
                linf = std::max(linf, dmax / Uin);
                sumAbs += std::sqrt(d2) / Uin;
                sumDiff2 += d2;
                sumRef2 += r2;
                ++fluidN;
            }
    std::printf("cuda vs cpu: Linf/Uin=%.4g  meanRel=%.4g  L2rel=%.4g\n", linf,
                sumAbs / static_cast<double>(fluidN), std::sqrt(sumDiff2 / sumRef2));

    CF_CHECK(linf < 0.05);
    CF_CHECK(std::sqrt(sumDiff2 / sumRef2) < 0.02);
    CF_CHECK(sumAbs / static_cast<double>(fluidN) < 0.01);
    CF_CHECK(solidMax < 0.1 * Uin);

    if (cftest::failures == 0) std::printf("cuda_wind_tunnel: all checks passed\n");
    return cftest::failures;
}
