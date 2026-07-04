// Implementation of the Cyberfluids C ABI (include/cyberfluids/capi.h).
// Wraps the C++20 solver behind a stable extern "C" surface.

#include "cyberfluids/capi.h"

#include <array>
#include <cstdint>

#include "cyberfluids/solver/lid_driven_cavity.hpp"

#ifndef CYBERFLUIDS_VERSION
#define CYBERFLUIDS_VERSION "0.0.0"
#endif

namespace {
using Cavity = cyberfluids::solver::LidDrivenCavity2D<>;
}

struct cf_cavity2d {
    Cavity impl;
    cf_cavity2d(std::int64_t nx, std::int64_t ny, double omega, double lidU)
        : impl(nx, ny, omega, lidU) {}
};

extern "C" {

const char *cf_version(void) { return CYBERFLUIDS_VERSION; }

cf_cavity2d *cf_cavity2d_create(int64_t nx, int64_t ny, double omega, double lid_velocity) {
    if (nx <= 0 || ny <= 0) return nullptr;
    return new cf_cavity2d(nx, ny, omega, lid_velocity);
}

void cf_cavity2d_destroy(cf_cavity2d *handle) { delete handle; }

void cf_cavity2d_run(cf_cavity2d *handle, int64_t steps) { handle->impl.run(steps); }

void cf_cavity2d_step(cf_cavity2d *handle) { handle->impl.step(); }

int64_t cf_cavity2d_nx(const cf_cavity2d *handle) { return handle->impl.nx(); }
int64_t cf_cavity2d_ny(const cf_cavity2d *handle) { return handle->impl.ny(); }

void cf_cavity2d_velocity(cf_cavity2d *handle, double *out_u) {
    const std::int64_t nx = handle->impl.nx(), ny = handle->impl.ny();
    for (std::int64_t x = 0; x < nx; ++x)
        for (std::int64_t y = 0; y < ny; ++y) {
            const std::array<double, 2> u = handle->impl.velocity(x, y);
            out_u[(x * ny + y) * 2 + 0] = u[0];
            out_u[(x * ny + y) * 2 + 1] = u[1];
        }
}

void cf_cavity2d_density(cf_cavity2d *handle, double *out_rho) {
    const std::int64_t nx = handle->impl.nx(), ny = handle->impl.ny();
    for (std::int64_t x = 0; x < nx; ++x)
        for (std::int64_t y = 0; y < ny; ++y)
            out_rho[x * ny + y] = handle->impl.density(x, y);
}

}  // extern "C"
