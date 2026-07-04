/// Metal GPU cavity validation: the same D2Q9 BGK lid-driven cavity run on the
/// GPU must (1) be deterministic run-to-run and (2) match the CPU (fp64) solver
/// within an fp32 tolerance. Skips cleanly if no Metal device is present.

#include <array>
#include <cmath>
#include <cstdint>

#include "cyberfluids/solver/lid_driven_cavity.hpp"
#include "cyberfluids/solver/metal/lid_driven_cavity_metal.hpp"
#include "testing.hpp"

using MetalCav = cyberfluids::solver::MetalLidDrivenCavity2D;

int main() {
    if (!MetalCav::metalAvailable()) {
        std::printf("metal_cavity: no Metal device, skipping (pass)\n");
        return 0;
    }

    const std::int64_t N = 64, steps = 5000;
    const double omega = 1.0 / 0.6, U = 0.05;

    // (1) Determinism: two identical GPU runs are bit-identical.
    {
        MetalCav a(N, N, omega, U), b(N, N, omega, U);
        a.run(steps);
        b.run(steps);
        const auto pa = a.downloadPopulations();
        const auto pb = b.downloadPopulations();
        bool same = (pa.size() == pb.size());
        for (std::size_t i = 0; i < pa.size() && same; ++i)
            if (pa[i] != pb[i]) same = false;
        CF_CHECK(same);
    }

    // (2) CPU(fp64) vs Metal(fp32) steady-state velocity field.
    MetalCav gpu(N, N, omega, U);
    gpu.run(steps);
    cyberfluids::solver::LidDrivenCavity2D<> cpu(N, N, omega, U);
    cpu.run(steps);

    double linf = 0.0, sumSq = 0.0;
    int n = 0;
    for (std::int64_t x = 0; x < N; ++x)
        for (std::int64_t y = 0; y < N; ++y) {
            const auto ug = gpu.velocity(x, y);
            const auto uc = cpu.velocity(x, y);
            const double dx = (ug[0] - uc[0]) / U, dy = (ug[1] - uc[1]) / U;
            linf = std::max(linf, std::max(std::fabs(dx), std::fabs(dy)));
            sumSq += dx * dx + dy * dy;
            n += 2;
        }
    const double l2 = std::sqrt(sumSq / n);
    std::printf("metal_cavity vs CPU (fraction of U): Linf=%.4f  L2=%.4f\n", linf, l2);
    CF_CHECK(linf < 0.05);
    CF_CHECK(l2 < 0.02);

    if (cftest::failures == 0) std::printf("metal_cavity: all checks passed\n");
    return cftest::failures;
}
