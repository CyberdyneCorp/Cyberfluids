/// Validates advection-diffusion dynamics on D2Q5: a sinusoidal scalar decays at
/// the analytic diffusion rate (u=0), and a scalar blob is advected by a uniform
/// external velocity (centroid tracks u*t). Uses a periodic 1D-in-x channel.

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/dynamics/advection_diffusion.hpp"
#include "cyberfluids/timestep/evolve.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::AdvectedD2Q5;
using D = AdvectedD2Q5;
using AD = cyberfluids::AdvectionDiffusionBGKdynamics<double, D>;
using Lattice = cyberfluids::BlockLattice2D<double, D>;
using Pop = cyberfluids::PopulationField<double, D>;

namespace {
constexpr double PI = 3.14159265358979323846;

Lattice makeLattice(std::int64_t nx, double omega, std::array<double, 2> u) {
    Lattice lat(nx, 1);
    lat.attributeDynamics(lat.getBoundingBox(), std::make_shared<AD>(omega));
    for (std::int64_t c = 0; c < lat.ncells(); ++c) {
        lat.externalField()(0, c) = u[0];
        lat.externalField()(1, c) = u[1];
    }
    return lat;
}
void initPhi(Lattice& lat, const std::vector<double>& phi, std::array<double, 2> u) {
    for (std::int64_t x = 0; x < lat.extent(0); ++x) {
        const std::int64_t c = lat.index(x, 0);
        for (int i = 0; i < 5; ++i)
            lat.populations()(i, c) = AD::equilibrium(i, phi[static_cast<std::size_t>(x)], u);
    }
}
double phiAt(Lattice& lat, std::int64_t x) {
    auto cell = lat.get(x, 0);
    double phi = 0;
    for (int i = 0; i < 5; ++i) phi += cell[i];
    return phi;
}
}  // namespace

int main() {
    // --- Pure diffusion: sinusoid decays as exp(-D k^2 t) ---
    {
        const std::int64_t nx = 64;
        const double omega = 1.0, A0 = 0.01;
        const double Dcoef = AD::diffusivity(omega);
        const double k = 2.0 * PI / nx;
        Lattice lat = makeLattice(nx, omega, {0.0, 0.0});
        std::vector<double> phi0(nx);
        for (std::int64_t x = 0; x < nx; ++x) phi0[x] = 1.0 + A0 * std::sin(k * x);
        initPhi(lat, phi0, {0.0, 0.0});

        const std::int64_t T = 200;
        Pop scratch(lat.ncells());
        for (std::int64_t s = 0; s < T; ++s) cyberfluids::collideAndStream(lat, scratch);

        // Discrete sine projection of the amplitude.
        double amp = 0.0;
        for (std::int64_t x = 0; x < nx; ++x) amp += (phiAt(lat, x) - 1.0) * std::sin(k * x);
        amp *= 2.0 / nx;
        const double expected = A0 * std::exp(-Dcoef * k * k * T);
        std::printf("AD diffusion: amp=%.6e expected=%.6e (D=%.4f)\n", amp, expected, Dcoef);
        CF_CHECK_CLOSE(amp, expected, 0.05 * A0);  // within 5% of the initial amplitude
    }

    // --- Advection: blob centroid moves by u*t ---
    {
        const std::int64_t nx = 64;
        const double omega = 1.0, Ux = 0.05;
        Lattice lat = makeLattice(nx, omega, {Ux, 0.0});
        std::vector<double> phi0(nx);
        const double x0 = 16.0, sigma = 4.0;
        for (std::int64_t x = 0; x < nx; ++x)
            phi0[x] = 0.1 + std::exp(-((x - x0) * (x - x0)) / (2.0 * sigma * sigma));
        initPhi(lat, phi0, {Ux, 0.0});

        const std::int64_t T = 200;
        Pop scratch(lat.ncells());
        for (std::int64_t s = 0; s < T; ++s) cyberfluids::collideAndStream(lat, scratch);

        // Centroid of (phi - floor); no wraparound since 16 + 0.05*200 = 26 < 64.
        double num = 0.0, den = 0.0;
        for (std::int64_t x = 0; x < nx; ++x) {
            const double w = phiAt(lat, x) - 0.1;
            num += x * w;
            den += w;
        }
        const double centroid = num / den;
        const double expected = x0 + Ux * T;
        std::printf("AD advection: centroid=%.3f expected=%.3f\n", centroid, expected);
        CF_CHECK(std::fabs(centroid - expected) < 1.5);
    }

    if (cftest::failures == 0) std::printf("advection_diffusion: all checks passed\n");
    return cftest::failures;
}
