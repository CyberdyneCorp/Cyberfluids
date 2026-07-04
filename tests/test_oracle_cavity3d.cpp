/// 3D oracle regression: the Cyberfluids D3Q19 lid-driven cavity must agree with
/// the stored Palabos reference on the interior centerline velocity profiles,
/// within a documented fp tolerance (physics-level — the codes use different
/// wall BCs). Parameters MUST match tests/oracle/palabos/cavity3d_oracle.cpp.

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
    double ux, uy, uz;
};
constexpr std::int64_t N = 20;
constexpr double U = 0.05;
const double OMEGA = 1.0 / 0.6;
constexpr std::int64_t ITERS = 8000;
// Tolerances as a fraction of U. The L∞ peak is the interior node just below the
// lid, where moving-wall bounce-back (Cyberfluids) and the Palabos interp BC
// differ most — proportionally larger at this coarse N=20 (observed ~0.13). The
// bulk agreement (L2, RMS over interior) is the real quality signal (~0.018).
constexpr double TOL_LINF = 0.16;
constexpr double TOL_L2 = 0.03;
}  // namespace

int main() {
    std::ifstream in(CYBERFLUIDS_ORACLE_DATA3D);
    CF_CHECK(in.good());
    if (!in.good()) return 1;

    std::string line;
    std::getline(in, line);  // header
    std::vector<Row> ref;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string a, c, ux, uy, uz;
        std::getline(ss, a, ',');
        std::getline(ss, c, ',');
        std::getline(ss, ux, ',');
        std::getline(ss, uy, ',');
        std::getline(ss, uz, ',');
        ref.push_back({a, std::stoi(c), std::stod(ux), std::stod(uy), std::stod(uz)});
    }

    cyberfluids::solver::LidDrivenCavity3D<> cav(N, N, N, OMEGA, U);
    cav.run(ITERS);
    const std::int64_t xc = N / 2, yc = N / 2, zc = N / 2;

    double linf = 0.0, sumSq = 0.0;
    int n = 0;
    std::string peakAxis;
    int peakCoord = -1;
    for (const auto& r : ref) {
        if (r.coord <= 0 || r.coord >= N - 1) continue;  // interior nodes only
        const auto u = (r.axis == "vertical") ? cav.velocity(xc, yc, r.coord)
                                              : cav.velocity(r.coord, yc, zc);
        const double d0 = (u[0] - r.ux) / U, d1 = (u[1] - r.uy) / U, d2 = (u[2] - r.uz) / U;
        const double e = std::max(std::fabs(d0), std::max(std::fabs(d1), std::fabs(d2)));
        if (e > linf) {
            linf = e;
            peakAxis = r.axis;
            peakCoord = r.coord;
        }
        sumSq += d0 * d0 + d1 * d1 + d2 * d2;
        n += 3;
    }
    const double l2 = std::sqrt(sumSq / n);
    std::printf("oracle cavity3d vs Palabos (interior, fraction of U): Linf=%.4f (at %s %d)  L2=%.4f\n",
                linf, peakAxis.c_str(), peakCoord, l2);
    CF_CHECK(linf < TOL_LINF);
    CF_CHECK(l2 < TOL_L2);

    if (cftest::failures == 0) std::printf("oracle_cavity3d: agreement within tolerance\n");
    return cftest::failures;
}
