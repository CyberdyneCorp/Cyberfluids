/* Cyberfluids C ABI — a thin, stable C surface over the C++20 core, wrapped by
 * the Python (ctypes + NumPy) and Swift (SwiftPM) bindings. See
 * openspec/specs/language-bindings/spec.md.
 *
 * MVP surface: the 2D lid-driven cavity solver plus macroscopic field readout.
 */
#ifndef CYBERFLUIDS_CAPI_H
#define CYBERFLUIDS_CAPI_H

#include <stdint.h>

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

#ifdef __cplusplus
}
#endif

#endif /* CYBERFLUIDS_CAPI_H */
