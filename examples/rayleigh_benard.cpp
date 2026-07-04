/// Rayleigh-Bénard convection demo. Usage: rayleigh_benard [nx=128] [ny=64] [Ra=20000] [steps=40000]
/// Writes a VTK snapshot (temperature + velocity) openable in ParaView.

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "cyberfluids/solver/rayleigh_benard.hpp"

int main(int argc, char** argv) {
    const std::int64_t nx = (argc > 1) ? std::atoll(argv[1]) : 128;
    const std::int64_t ny = (argc > 2) ? std::atoll(argv[2]) : 64;
    const double Ra = (argc > 3) ? std::atof(argv[3]) : 20000.0;
    const std::int64_t steps = (argc > 4) ? std::atoll(argv[4]) : 40000;

    auto rb = cyberfluids::solver::RayleighBenard2D<>::fromDimensionless(nx, ny, Ra, 1.0, 0.1);
    std::printf("Rayleigh-Bénard: %lldx%lld, Ra=%.0f, Pr=1, steps=%lld\n",
                static_cast<long long>(nx), static_cast<long long>(ny), Ra,
                static_cast<long long>(steps));
    rb.perturbTemperature(1e-3);
    rb.run(steps);

    std::printf("mean kinetic energy = %.6e\n", rb.avgKineticEnergy());
    rb.writeVtk("rayleigh_benard.vtk");
    std::printf("wrote rayleigh_benard.vtk (open in ParaView)\n");
    return 0;
}
