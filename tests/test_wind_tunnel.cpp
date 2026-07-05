/// Validates the 3D wind tunnel with an analytic sphere obstacle (no CMG):
///   - the run stays finite (low-Mach stability — no blow-up);
///   - velocity inside the solid sphere is ~0 (no-slip via ns=1 bounce-back);
///   - free-stream is recovered far upstream;
///   - a momentum-deficit wake forms directly behind the sphere;
///   - flow at the sphere's shoulder is faster than in the wake (it accelerates
///     around the body).

#include <array>
#include <cmath>
#include <cstdint>

#include "cyberfluids/solver/wind_tunnel3d.hpp"
#include "testing.hpp"

int main() {
    const std::int64_t nx = 48, ny = 24, nz = 24;
    const double Uin = 0.05, R = 4.0, D = 2 * R;   // diameter = characteristic length
    const double cx = 14, cy = 12, cz = 12;        // sphere centre (upstream third)
    const double omega =
        cyberfluids::solver::WindTunnel3D<>::omegaForReynolds(Uin, D, /*Re=*/20.0);  // ~1.79

    cyberfluids::solver::WindTunnel3D<> tunnel(nx, ny, nz, omega, Uin);
    tunnel.setObstacleSphere(cx, cy, cz, R, /*sharp=*/true);
    tunnel.run(5000);

    // (1) Stability: every cell speed is finite.
    bool allFinite = true;
    double maxSpeed = 0.0;
    for (std::int64_t x = 0; x < nx; ++x)
        for (std::int64_t y = 0; y < ny; ++y)
            for (std::int64_t z = 0; z < nz; ++z) {
                const auto u = tunnel.velocity(x, y, z);
                const double sp = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
                if (!std::isfinite(sp)) allFinite = false;
                maxSpeed = std::max(maxSpeed, sp);
            }
    CF_CHECK(allFinite);
    CF_CHECK(maxSpeed < 2.5 * Uin);  // tight low-Mach bound (a working sphere peaks ~1.2 Uin)

    // Probe points (all fluid except the centre; distances from centre (14,12,12)).
    const auto uSolid = tunnel.velocity(14, 12, 12);     // inside the sphere
    const auto uUpstream = tunnel.velocity(4, 6, 6);      // far upstream, off-axis (d=13)
    const auto uFront = tunnel.velocity(9, 12, 12);       // on-axis just ahead of nose (d=5)
    const auto uWake = tunnel.velocity(20, 12, 12);       // directly behind (d=6)
    const auto uShoulder = tunnel.velocity(14, 18, 12);   // beside, at widest (d=6)
    const auto uDeflect = tunnel.velocity(10, 16, 12);    // front-upper, flow turning (d=5.7)

    const double solidSpeed =
        std::sqrt(uSolid[0] * uSolid[0] + uSolid[1] * uSolid[1] + uSolid[2] * uSolid[2]);
    const double up = uUpstream[0];

    // (2) Free-stream recovered far upstream; no-slip inside the solid.
    CF_CHECK(up > 0.8 * Uin);
    CF_CHECK(solidSpeed < 0.1 * Uin);

    // The three checks below are load-bearing: a broken/no-op solver that leaves
    // the initial uniform u=(Uin,0,0) field would FAIL every one of them.
    // (3) Flow decelerates into the front stagnation region.
    CF_CHECK(uFront[0] < 0.5 * up);
    // (4) A momentum-deficit wake forms behind the body.
    CF_CHECK(uWake[0] < 0.5 * up);
    CF_CHECK(uShoulder[0] > uWake[0]);  // faster at the shoulder than in the wake
    // (5) Flow is deflected transversely around the body (uniform flow has vy=0).
    CF_CHECK(std::fabs(uDeflect[1]) > 0.1 * Uin);

    if (cftest::failures == 0) std::printf("wind_tunnel: all checks passed\n");
    return cftest::failures;
}
