/// Force-driven Poiseuille channel demo. Usage: poiseuille2d [ny=64] [steps=30000]
/// Prints the simulated vs analytic centerline velocity (they should match).

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "cyberfluids/solver/poiseuille_channel.hpp"

int main(int argc, char** argv) {
    const std::int64_t ny = (argc > 1) ? std::atoll(argv[1]) : 64;
    const std::int64_t steps = (argc > 2) ? std::atoll(argv[2]) : 30000;
    const std::int64_t nx = 8;
    const double omega = 1.0, forceX = 1e-5;

    cyberfluids::solver::PoiseuilleChannel2D<> chan(nx, ny, omega, forceX);
    std::printf("Poiseuille channel: %lldx%lld, omega=%.3f, Fx=%.1e, steps=%lld\n",
                static_cast<long long>(nx), static_cast<long long>(ny), omega, forceX,
                static_cast<long long>(steps));
    chan.run(steps);

    double linf = 0.0, uMax = 0.0;
    for (std::int64_t y = 0; y < ny; ++y) uMax = std::max(uMax, chan.analyticUx(y, omega));
    for (std::int64_t y = 0; y < ny; ++y)
        linf = std::max(linf, std::fabs(chan.velocity(0, y)[0] - chan.analyticUx(y, omega)) / uMax);

    const std::int64_t yc = ny / 2;
    std::printf("center: sim u_x=%.6e  analytic=%.6e  Linf(rel)=%.2e\n",
                chan.velocity(0, yc)[0], chan.analyticUx(yc, omega), linf);
    return 0;
}
