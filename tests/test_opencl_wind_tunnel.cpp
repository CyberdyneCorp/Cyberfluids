/// Validates the OpenCL wind tunnel (fp32, GPU) against the CPU solver::WindTunnel3D
/// (fp64) oracle on the same case, plus determinism and no-slip. Skips cleanly
/// (exit 0) when no OpenCL device is available. Runs on this machine's GPU.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "cyberfluids/solver/opencl/wind_tunnel_opencl.hpp"
#include "cyberfluids/solver/wind_tunnel3d.hpp"
#include "testing.hpp"

using cyberfluids::solver::WindTunnel3D;
using cyberfluids::solver::WindTunnelOpenCL;

int main() {
    if (!WindTunnelOpenCL::openclAvailable()) {
        std::printf("SKIP opencl_wind_tunnel: no OpenCL device\n");
        return 0;
    }

    const std::int64_t nx = 48, ny = 24, nz = 24;
    const double Uin = 0.05, R = 4.0;
    const double cx = 14, cy = 12, cz = 12;
    const double omega = WindTunnel3D<>::omegaForReynolds(Uin, 2 * R, 20.0);
    const std::int64_t steps = 5000;

    // fp64 CPU oracle.
    WindTunnel3D<> cpu(nx, ny, nz, omega, Uin);
    cpu.setObstacleSphere(cx, cy, cz, R, true);
    cpu.run(steps);

    // fp32 GPU.
    WindTunnelOpenCL gpu(nx, ny, nz, omega, Uin);
    gpu.setObstacleSphere(cx, cy, cz, R, true);
    gpu.run(steps);

    // (1) Obstacle fidelity: ns computed with identical host math -> exact match.
    bool nsExact = true;
    for (std::int64_t x = 0; x < nx; ++x)
        for (std::int64_t y = 0; y < ny; ++y)
            for (std::int64_t z = 0; z < nz; ++z)
                if (gpu.solidFraction(x, y, z) != cpu.solidFraction(x, y, z)) nsExact = false;
    CF_CHECK(nsExact);

    // (2) Determinism: a second identical GPU run is bit-for-bit equal.
    WindTunnelOpenCL gpu2(nx, ny, nz, omega, Uin);
    gpu2.setObstacleSphere(cx, cy, cz, R, true);
    gpu2.run(steps);
    const auto p1 = gpu.downloadPopulations(), p2 = gpu2.downloadPopulations();
    bool deterministic = (p1.size() == p2.size());
    for (std::size_t i = 0; i < p1.size() && deterministic; ++i)
        if (p1[i] != p2[i]) deterministic = false;
    CF_CHECK(deterministic);

    // (3) GPU (fp32) vs CPU (fp64) velocity field over fluid cells, normalized by Uin.
    double linf = 0.0, sumDiff2 = 0.0, sumRef2 = 0.0, sumAbs = 0.0, solidMax = 0.0;
    std::int64_t fluidN = 0;
    for (std::int64_t x = 0; x < nx; ++x)
        for (std::int64_t y = 0; y < ny; ++y)
            for (std::int64_t z = 0; z < nz; ++z) {
                const auto ug = gpu.velocity(x, y, z);
                if (cpu.solidFraction(x, y, z) > 0.5) {
                    const double sp = std::sqrt(ug[0] * ug[0] + ug[1] * ug[1] + ug[2] * ug[2]);
                    solidMax = std::max(solidMax, sp);
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
    const double meanRel = sumAbs / static_cast<double>(fluidN);
    const double l2Rel = std::sqrt(sumDiff2 / sumRef2);

    std::printf("opencl vs cpu: Linf/Uin=%.4g  meanRel=%.4g  L2rel=%.4g  solidMax/Uin=%.4g\n",
                linf, meanRel, l2Rel, solidMax / Uin);

    CF_CHECK(linf < 0.05);          // fp32-vs-fp64, matches the Metal-cavity bar
    CF_CHECK(l2Rel < 0.02);         // field-relative L2 agreement < 2%
    CF_CHECK(meanRel < 0.01);       // mean per-cell error < 1% of Uin
    CF_CHECK(solidMax < 0.1 * Uin);  // no-slip preserved inside the solid

    if (cftest::failures == 0) std::printf("opencl_wind_tunnel: all checks passed\n");
    return cftest::failures;
}
