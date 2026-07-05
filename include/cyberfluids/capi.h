/* Cyberfluids C ABI — a thin, stable C surface over the C++20 core, wrapped by
 * the Python (ctypes + NumPy) and Swift (SwiftPM) bindings. See
 * openspec/specs/language-bindings/spec.md.
 *
 * MVP surface: the 2D lid-driven cavity solver plus macroscopic field readout.
 */
#ifndef CYBERFLUIDS_CAPI_H
#define CYBERFLUIDS_CAPI_H

#include <stdint.h>

#include "cyberfluids/interop/dlpack.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library version string, e.g. "0.0.1". */
const char *cf_version(void);

/* Opaque handle to a 2D lid-driven cavity (D2Q9, BGK). */
typedef struct cf_cavity2d cf_cavity2d;

/* Create a cavity on an nx*ny grid with relaxation omega and lid velocity.
 * Returns NULL on invalid arguments (nx<=0 || ny<=0). */
cf_cavity2d *cf_cavity2d_create(int64_t nx, int64_t ny, double omega, double lid_velocity);

/* Release a cavity created by cf_cavity2d_create. Safe on NULL. */
void cf_cavity2d_destroy(cf_cavity2d *handle);

/* Advance the simulation by `steps` collide-and-stream iterations. */
void cf_cavity2d_run(cf_cavity2d *handle, int64_t steps);

/* Advance the simulation by a single iteration. */
void cf_cavity2d_step(cf_cavity2d *handle);

int64_t cf_cavity2d_nx(const cf_cavity2d *handle);
int64_t cf_cavity2d_ny(const cf_cavity2d *handle);

/* Write the velocity field into out_u (length nx*ny*2, C order:
 * out_u[(x*ny + y)*2 + component]). */
void cf_cavity2d_velocity(cf_cavity2d *handle, double *out_u);

/* Write the density field into out_rho (length nx*ny, C order:
 * out_rho[x*ny + y]). */
void cf_cavity2d_density(cf_cavity2d *handle, double *out_rho);

/* Zero-copy DLPack export. Each returns a heap DLManagedTensor aliasing a live
 * NumPP buffer (no copy). The caller owns the pointer and MUST call its deleter
 * exactly once; a DLPack consumer (numpy/torch from_dlpack) does this when the
 * imported view is released. Returns NULL on a NULL handle.
 *
 * populations: shape {9, nx*ny} float64 — aliases the live populations buffer.
 *   NOTE: step() swaps the populations buffer, so a tensor exported here becomes
 *   stale after the next step; export it after the final step, or re-export.
 * velocity/density: refresh the in-place macroscopic cache, then alias it. These
 *   caches are stable across steps (re-call after stepping to see fresh values). */
DLManagedTensor *cf_cavity2d_populations_dlpack(cf_cavity2d *handle);
DLManagedTensor *cf_cavity2d_velocity_dlpack(cf_cavity2d *handle);
DLManagedTensor *cf_cavity2d_density_dlpack(cf_cavity2d *handle);

/* ---- 3D wind tunnel: external flow past an obstacle (D3Q19) ---- */

/* Opaque handle to a 3D wind tunnel. */
typedef struct cf_wind_tunnel cf_wind_tunnel;

/* Create an empty tunnel of the given grid with relaxation omega and free-stream
 * inflow speed (lattice units). Add an obstacle with cf_wind_tunnel_set_sphere or
 * cf_wind_tunnel_create_from_stl. Returns NULL on invalid dimensions. */
cf_wind_tunnel *cf_wind_tunnel_create(int64_t nx, int64_t ny, int64_t nz, double omega,
                                      double inflow);

/* Load an STL/OBJ, size a tunnel around it (pads in cells: upstream, downstream,
 * lateral), and embed the voxelized obstacle. `resolution` = obstacle cells along
 * its longest axis. Returns NULL when the build lacks STL support (see
 * cf_geometry_available) or on read failure. */
cf_wind_tunnel *cf_wind_tunnel_create_from_stl(const char *path, int resolution, int pad_up,
                                               int pad_down, int pad_lat, double omega,
                                               double inflow);

/* Set a solid sphere obstacle (centre and radius in lattice cells). */
void cf_wind_tunnel_set_sphere(cf_wind_tunnel *handle, double cx, double cy, double cz,
                               double radius);

void cf_wind_tunnel_run(cf_wind_tunnel *handle, int64_t steps);
void cf_wind_tunnel_destroy(cf_wind_tunnel *handle);

int64_t cf_wind_tunnel_nx(const cf_wind_tunnel *handle);
int64_t cf_wind_tunnel_ny(const cf_wind_tunnel *handle);
int64_t cf_wind_tunnel_nz(const cf_wind_tunnel *handle);

/* Velocity field into out_u (length nx*ny*nz*3, C order
 * out_u[((x*ny+y)*nz+z)*3 + component]). */
void cf_wind_tunnel_velocity(cf_wind_tunnel *handle, double *out_u);
/* Solid fraction into out_ns (length nx*ny*nz, same cell order). */
void cf_wind_tunnel_solid(cf_wind_tunnel *handle, double *out_ns);
/* Write a legacy-VTK STRUCTURED_POINTS file (velocity, speed, solid) for ParaView. */
void cf_wind_tunnel_write_vtk(cf_wind_tunnel *handle, const char *path);

/* 1 if the library was built with STL geometry support, else 0. */
int cf_geometry_available(void);

#ifdef __cplusplus
}
#endif

#endif /* CYBERFLUIDS_CAPI_H */
