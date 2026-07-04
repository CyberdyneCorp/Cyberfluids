/// 3D lid-driven cavity demo. Usage: cavity3d [n=48] [steps=5000]

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "cyberfluids/solver/lid_driven_cavity.hpp"

int main(int argc, char** argv) {
    const std::int64_t n = (argc > 1) ? std::atoll(argv[1]) : 48;
    const std::int64_t steps = (argc > 2) ? std::atoll(argv[2]) : 5000;
    const double U = 0.1, omega = 1.0;

    cyberfluids::solver::LidDrivenCavity3D<> cav(n, n, n, omega, U);
    std::printf("3D lid-driven cavity: %lld^3, omega=%.3f, U=%.3f, steps=%lld\n",
                static_cast<long long>(n), omega, U, static_cast<long long>(steps));

    cav.run(steps);

    const auto uc = cav.velocity(n / 2, n / 2, n / 2);
    std::printf("center velocity = (%.6f, %.6f, %.6f)\n", uc[0], uc[1], uc[2]);
    return 0;
}
