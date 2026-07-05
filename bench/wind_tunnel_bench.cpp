// Throughput benchmark for the D3Q19 wind-tunnel solver across backends:
//   CPU (fp64, std::execution::par_unseq)  vs  CUDA (fp32)  vs  OpenCL (fp32).
//
// Reports GLUPS = ncells * steps / seconds / 1e9 (giga lattice-updates per
// second; higher is better) on an identical run, plus the speed-up over the CPU
// baseline. GPU backends are compiled in only when their CMake flag is set, so
// this same file builds everywhere — CPU-only hosts still get a CPU number.
//
//   Usage: wind_tunnel_bench [nx] [ny] [nz] [steps]   (defaults: 160 80 80 500)

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "cyberfluids/solver/wind_tunnel3d.hpp"
#if defined(CYBERFLUIDS_WITH_CUDA)
#include "cyberfluids/solver/cuda/wind_tunnel_cuda.hpp"
#endif
#if defined(CYBERFLUIDS_WITH_OPENCL)
#include "cyberfluids/solver/opencl/wind_tunnel_opencl.hpp"
#endif

using cyberfluids::solver::WindTunnel3D;
using Clock = std::chrono::steady_clock;

namespace {

double glups(std::int64_t cells, std::int64_t steps, double sec) {
    return static_cast<double>(cells) * static_cast<double>(steps) / sec / 1e9;
}

// Warm up (JIT, allocations, first-touch), then time `timed` steps. The trailing
// velocity() read forces any pending device->host sync into the timed window so
// GPU numbers are not flattered by unsynced, still-queued work.
template <class Solver>
double timeRun(Solver& s, std::int64_t warm, std::int64_t timed) {
    s.run(warm);
    (void)s.velocity(1, 1, 1);
    const auto t0 = Clock::now();
    s.run(timed);
    (void)s.velocity(1, 1, 1);
    return std::chrono::duration<double>(Clock::now() - t0).count();
}

template <class Solver>
double benchOne(const char* name, std::int64_t nx, std::int64_t ny, std::int64_t nz,
                double omega, double Uin, double R, double cx, double cy, double cz,
                std::int64_t warm, std::int64_t steps, double cpuGlups) {
    Solver s(nx, ny, nz, omega, Uin);
    s.setObstacleSphere(cx, cy, cz, R, true);
    const double sec = timeRun(s, warm, steps);
    const double g = glups(nx * ny * nz, steps, sec);
    if (cpuGlups > 0.0)
        std::printf("%-12s %12.4f %12.4f %8.1fx\n", name, sec, g, g / cpuGlups);
    else
        std::printf("%-12s %12.4f %12.4f %9s\n", name, sec, g, "1.0x");
    return g;
}

}  // namespace

int main(int argc, char** argv) {
    const std::int64_t nx = argc > 1 ? std::atoll(argv[1]) : 160;
    const std::int64_t ny = argc > 2 ? std::atoll(argv[2]) : 80;
    const std::int64_t nz = argc > 3 ? std::atoll(argv[3]) : 80;
    const std::int64_t steps = argc > 4 ? std::atoll(argv[4]) : 500;
    const std::int64_t warm = 50;

    const double Uin = 0.05, R = nx / 8.0, cx = nx / 4.0, cy = ny / 2.0, cz = nz / 2.0;
    const double omega = WindTunnel3D<>::omegaForReynolds(Uin, 2 * R, 100.0);

    std::printf("grid %lldx%lldx%lld = %lld cells | %lld timed steps (warm %lld)\n",
                (long long)nx, (long long)ny, (long long)nz, (long long)(nx * ny * nz),
                (long long)steps, (long long)warm);
    std::printf("%-12s %12s %12s %10s\n", "backend", "time (s)", "GLUPS", "vs CPU");

    const double cpu = benchOne<WindTunnel3D<>>("CPU-fp64", nx, ny, nz, omega, Uin, R, cx, cy, cz,
                                                warm, steps, 0.0);
#if defined(CYBERFLUIDS_WITH_CUDA)
    if (cyberfluids::solver::WindTunnelCuda::cudaAvailable())
        benchOne<cyberfluids::solver::WindTunnelCuda>("CUDA-fp32", nx, ny, nz, omega, Uin, R, cx,
                                                      cy, cz, warm, steps, cpu);
    else
        std::printf("CUDA-fp32    (skipped — no CUDA device)\n");
#endif
#if defined(CYBERFLUIDS_WITH_OPENCL)
    if (cyberfluids::solver::WindTunnelOpenCL::openclAvailable())
        benchOne<cyberfluids::solver::WindTunnelOpenCL>("OpenCL-fp32", nx, ny, nz, omega, Uin, R,
                                                        cx, cy, cz, warm, steps, cpu);
    else
        std::printf("OpenCL-fp32  (skipped — no OpenCL device)\n");
#endif
    return 0;
}
