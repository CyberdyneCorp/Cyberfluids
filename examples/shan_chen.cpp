/// Shan-Chen phase-separation demo. Usage: shan_chen [n=128] [G=-5] [steps=8000]
/// Seeds a near-uniform density and lets it de-mix; writes a VTK density snapshot.

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "cyberfluids/io/vtk_writer.hpp"
#include "cyberfluids/solver/shan_chen_2d.hpp"

int main(int argc, char** argv) {
    const std::int64_t n = (argc > 1) ? std::atoll(argv[1]) : 128;
    const double G = (argc > 2) ? std::atof(argv[2]) : -5.0;
    const std::int64_t steps = (argc > 3) ? std::atoll(argv[3]) : 8000;

    cyberfluids::solver::ShanChen2D<> sc(n, n, 1.0, {G, 1.0});
    sc.initDensity(1.0, 0.01);
    std::printf("Shan-Chen: %lldx%lld, G=%.2f, steps=%lld\n", static_cast<long long>(n),
                static_cast<long long>(n), G, static_cast<long long>(steps));
    sc.run(steps);

    const auto mm = sc.minMaxDensity();
    std::printf("density range: [%.4f, %.4f]\n", mm[0], mm[1]);

    cyberfluids::io::VtkStructuredWriter<2> w(n, n);
    w.addScalar("density", [&](std::array<std::int64_t, 2> c) { return sc.density(c[0], c[1]); });
    w.write("shan_chen.vtk");
    std::printf("wrote shan_chen.vtk (open in ParaView)\n");
    return 0;
}
