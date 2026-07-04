/// 2D lid-driven cavity demo. Usage: cavity2d [n=128] [steps=20000]
/// Writes centerline velocity profiles to cavity2d_centerlines.csv.

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "cyberfluids/solver/lid_driven_cavity.hpp"

int main(int argc, char** argv) {
    const std::int64_t n = (argc > 1) ? std::atoll(argv[1]) : 128;
    const std::int64_t steps = (argc > 2) ? std::atoll(argv[2]) : 20000;
    const double U = 0.1, omega = 1.0;

    cyberfluids::solver::LidDrivenCavity2D<> cav(n, n, omega, U);
    std::printf("2D lid-driven cavity: %lldx%lld, omega=%.3f, U=%.3f, steps=%lld\n",
                static_cast<long long>(n), static_cast<long long>(n), omega, U,
                static_cast<long long>(steps));

    cav.run(steps);

    const auto uc = cav.velocity(n / 2, n / 2);
    std::printf("center velocity = (%.6f, %.6f)\n", uc[0], uc[1]);
    cav.writeCenterlines("cavity2d_centerlines.csv");
    std::printf("wrote cavity2d_centerlines.csv\n");
    return 0;
}
