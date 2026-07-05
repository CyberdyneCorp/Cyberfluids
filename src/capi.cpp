// Implementation of the Cyberfluids C ABI (include/cyberfluids/capi.h).
// Wraps the C++20 solver behind a stable extern "C" surface.

#include "cyberfluids/capi.h"

#include <array>
#include <cstdint>

#include "cyberfluids/interop/dlpack_export.hpp"
#include "cyberfluids/solver/lid_driven_cavity.hpp"
#include "cyberfluids/solver/wind_tunnel3d.hpp"
#ifdef CYBERFLUIDS_WITH_GEOMETRY
#include "cyberfluids/geometry/cmg_loader.hpp"
#endif

#ifndef CYBERFLUIDS_VERSION
#define CYBERFLUIDS_VERSION "0.0.0"
#endif

namespace {
using Cavity = cyberfluids::solver::LidDrivenCavity2D<>;
using Tunnel = cyberfluids::solver::WindTunnel3D<>;
}

struct cf_wind_tunnel {
    Tunnel impl;
    cf_wind_tunnel(std::int64_t nx, std::int64_t ny, std::int64_t nz, double omega, double inflow)
        : impl(nx, ny, nz, omega, inflow) {}
};

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

DLManagedTensor *cf_cavity2d_populations_dlpack(cf_cavity2d *handle) {
    if (!handle) return nullptr;
    return cyberfluids::interop::to_dlpack(handle->impl.lattice().populations().array());
}

DLManagedTensor *cf_cavity2d_velocity_dlpack(cf_cavity2d *handle) {
    if (!handle) return nullptr;
    handle->impl.refreshMacroscopic();
    return cyberfluids::interop::to_dlpack(handle->impl.velocityField().array());
}

DLManagedTensor *cf_cavity2d_density_dlpack(cf_cavity2d *handle) {
    if (!handle) return nullptr;
    handle->impl.refreshMacroscopic();
    return cyberfluids::interop::to_dlpack(handle->impl.densityField().array());
}

// ---- 3D wind tunnel ----

cf_wind_tunnel *cf_wind_tunnel_create(int64_t nx, int64_t ny, int64_t nz, double omega,
                                      double inflow) {
    // Each extent must be >= 3 (distinct inlet/interior/outlet planes). Validate
    // here so the constructor's exception never crosses the C ABI.
    if (nx < 3 || ny < 3 || nz < 3) return nullptr;
    return new cf_wind_tunnel(nx, ny, nz, omega, inflow);
}

cf_wind_tunnel *cf_wind_tunnel_create_from_stl(const char *path, int resolution, int pad_up,
                                               int pad_down, int pad_lat, double omega,
                                               double inflow) {
#ifdef CYBERFLUIDS_WITH_GEOMETRY
    if (!path || resolution < 1) return nullptr;
    try {
        const auto obj = cyberfluids::geometry::loadAndVoxelize(path, resolution, 1);
        const int64_t nx = pad_up + obj.nx + pad_down;
        const int64_t ny = obj.ny + 2 * pad_lat;
        const int64_t nz = obj.nz + 2 * pad_lat;
        if (nx < 3 || ny < 3 || nz < 3) return nullptr;  // reject bad pads/dims
        auto *h = new cf_wind_tunnel(nx, ny, nz, omega, inflow);
        h->impl.setObstacleField(obj, pad_up, pad_lat, pad_lat, /*sharp=*/false);
        return h;
    } catch (...) {
        return nullptr;  // read/voxelize failure
    }
#else
    (void)path; (void)resolution; (void)pad_up; (void)pad_down; (void)pad_lat; (void)omega;
    (void)inflow;
    return nullptr;  // built without STL geometry support
#endif
}

void cf_wind_tunnel_set_sphere(cf_wind_tunnel *handle, double cx, double cy, double cz,
                               double radius) {
    if (handle) handle->impl.setObstacleSphere(cx, cy, cz, radius, /*sharp=*/true);
}

void cf_wind_tunnel_run(cf_wind_tunnel *handle, int64_t steps) {
    if (handle) handle->impl.run(steps);
}
void cf_wind_tunnel_destroy(cf_wind_tunnel *handle) { delete handle; }

int64_t cf_wind_tunnel_nx(const cf_wind_tunnel *handle) { return handle->impl.nx(); }
int64_t cf_wind_tunnel_ny(const cf_wind_tunnel *handle) { return handle->impl.ny(); }
int64_t cf_wind_tunnel_nz(const cf_wind_tunnel *handle) { return handle->impl.nz(); }

void cf_wind_tunnel_velocity(cf_wind_tunnel *handle, double *out_u) {
    const int64_t nx = handle->impl.nx(), ny = handle->impl.ny(), nz = handle->impl.nz();
    for (int64_t x = 0; x < nx; ++x)
        for (int64_t y = 0; y < ny; ++y)
            for (int64_t z = 0; z < nz; ++z) {
                const std::array<double, 3> u = handle->impl.velocity(x, y, z);
                const int64_t base = ((x * ny + y) * nz + z) * 3;
                out_u[base + 0] = u[0];
                out_u[base + 1] = u[1];
                out_u[base + 2] = u[2];
            }
}

void cf_wind_tunnel_solid(cf_wind_tunnel *handle, double *out_ns) {
    const int64_t nx = handle->impl.nx(), ny = handle->impl.ny(), nz = handle->impl.nz();
    for (int64_t x = 0; x < nx; ++x)
        for (int64_t y = 0; y < ny; ++y)
            for (int64_t z = 0; z < nz; ++z)
                out_ns[(x * ny + y) * nz + z] = handle->impl.solidFraction(x, y, z);
}

void cf_wind_tunnel_write_vtk(cf_wind_tunnel *handle, const char *path) {
    if (handle && path) handle->impl.writeVtk(path);
}

int cf_geometry_available(void) {
#ifdef CYBERFLUIDS_WITH_GEOMETRY
    return 1;
#else
    return 0;
#endif
}

}  // extern "C"
