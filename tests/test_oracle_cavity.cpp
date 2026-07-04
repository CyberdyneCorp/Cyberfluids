/// Oracle regression test: the Cyberfluids 2D lid-driven cavity must agree with
/// the stored Palabos reference on the steady-state centerline velocity profiles
/// (interior nodes), within a documented tolerance. Because the two codes use
/// different velocity boundary schemes (Cyberfluids: moving-wall bounce-back;
/// Palabos: interpolation BC), this is a physics-level comparison, so the
/// boundary rows are excluded and the tolerance is in fractions of the lid speed.
///
/// Parameters MUST match tests/oracle/palabos/cavity2d_oracle.cpp.
/// The reference path is injected by CMake as CYBERFLUIDS_ORACLE_DATA.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cyberfluids/solver/lid_driven_cavity.hpp"
#include "testing.hpp"

namespace {
struct Row {
    std::string axis;
    int coord;
    double ux, uy;
};

// Matched discrete parameters.
constexpr std::int64_t N = 64;
constexpr double U = 0.05;
const double OMEGA = 1.0 / 0.6;
constexpr std::int64_t ITERS = 40000;

// Documented tolerances (fraction of lid speed U), set with margin above the
// observed deviation (L∞ ~= 0.050 just below the lid, L2 ~= 0.008 overall).
// The peak sits adjacent to the lid, where moving-wall bounce-back and the
// Palabos interpolation BC differ most. See tests/oracle/README.md.
constexpr double TOL_LINF = 0.07;   // 7% of U
constexpr double TOL_L2 = 0.015;    // 1.5% of U (RMS)
}  // namespace

int main() {
    std::ifstream in(CYBERFLUIDS_ORACLE_DATA);
    CF_CHECK(in.good());
    if (!in.good()) {
        std::printf("oracle: cannot open reference %s\n", CYBERFLUIDS_ORACLE_DATA);
        return 1;
    }

    std::string line;
    std::getline(in, line);  // header
    std::vector<Row> ref;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string a, c, ux, uy;
        std::getline(ss, a, ',');
        std::getline(ss, c, ',');
        std::getline(ss, ux, ',');
        std::getline(ss, uy, ',');
        ref.push_back({a, std::stoi(c), std::stod(ux), std::stod(uy)});
    }

    cyberfluids::solver::LidDrivenCavity2D<> cav(N, N, OMEGA, U);
    cav.run(ITERS);
    const std::int64_t xc = N / 2, yc = N / 2;

    double linf = 0.0, sumSq = 0.0;
    int n = 0;
    std::string peakAxis;
    int peakCoord = -1;
    for (const auto& r : ref) {
        if (r.coord <= 0 || r.coord >= N - 1) continue;  // interior nodes only
        std::array<double, 2> u = (r.axis == "vertical") ? cav.velocity(xc, r.coord)
                                                          : cav.velocity(r.coord, yc);
        const double dux = (u[0] - r.ux) / U;
        const double duy = (u[1] - r.uy) / U;
        const double e = std::max(std::fabs(dux), std::fabs(duy));
        if (e > linf) {
            linf = e;
            peakAxis = r.axis;
            peakCoord = r.coord;
        }
        sumSq += dux * dux + duy * duy;
        n += 2;
    }
    const double l2 = std::sqrt(sumSq / n);

    std::printf("oracle cavity2d vs Palabos (interior, fraction of U): Linf=%.4f (at %s coord=%d)  L2=%.4f\n",
                linf, peakAxis.c_str(), peakCoord, l2);
    CF_CHECK(linf < TOL_LINF);
    CF_CHECK(l2 < TOL_L2);

    if (cftest::failures == 0) std::printf("oracle: agreement within tolerance\n");
    return cftest::failures;
}
