#if defined(CYBERFLUIDS_WITH_OPENCL)

#include "cyberfluids/solver/opencl/wind_tunnel_opencl.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "cyberfluids/io/vtk_writer.hpp"

namespace cyberfluids::solver {

namespace {

// D3Q19 stencil on the host (must match include/cyberfluids/core/descriptors.hpp).
constexpr int kCx[19] = {0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0};
constexpr int kCy[19] = {0, 0, 0, 1, -1, 0, 0, 1, -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, -1};
constexpr int kCz[19] = {0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, -1, 1, 1, -1, -1, 1};
inline double weight(int i) {
    return i == 0 ? 1.0 / 3.0 : (i < 7 ? 1.0 / 18.0 : 1.0 / 36.0);
}

// Two OpenCL C kernels, faithful ports of PorousForcedBGKdynamics::collide
// (force=0) and WindTunnel3D::streamWithBoundaries. Same arithmetic shape as the
// fp64 CPU oracle so fp32 results agree within tolerance.
const char* kKernelSource = R"CL(
constant int cx[19]  = {0, 1,-1,0,0,0,0, 1,-1,1,-1, 1,-1,1,-1, 0,0,0,0};
constant int cy[19]  = {0, 0,0,1,-1,0,0, 1,-1,-1,1, 0,0,0,0, 1,-1,1,-1};
constant int cz[19]  = {0, 0,0,0,0,1,-1, 0,0,0,0, 1,-1,-1,1, 1,-1,-1,1};
constant int opp[19] = {0,2,1,4,3,6,5,8,7,10,9,12,11,14,13,16,15,18,17};
constant float wt[19] = {0.333333333f,
    0.055555556f,0.055555556f,0.055555556f,0.055555556f,0.055555556f,0.055555556f,
    0.027777778f,0.027777778f,0.027777778f,0.027777778f,0.027777778f,0.027777778f,
    0.027777778f,0.027777778f,0.027777778f,0.027777778f,0.027777778f,0.027777778f};

// Porous convex-blend collide, in place (force = 0).
kernel void collide(global float* f, global const float* ns, const long n, const float omega) {
    long g = get_global_id(0);
    if (g >= n) return;
    float fi[19];
    float rho = 0.f, jx = 0.f, jy = 0.f, jz = 0.f;
    for (int i = 0; i < 19; ++i) {
        float v = f[(long)i * n + g];   // snapshot: bounce-back reads pre-collision f[opp]
        fi[i] = v; rho += v;
        jx += v * cx[i]; jy += v * cy[i]; jz += v * cz[i];
    }
    float invRho = 1.f / rho;
    float ux = jx * invRho, uy = jy * invRho, uz = jz * invRho;
    float uSqr = ux * ux + uy * uy + uz * uz;
    float s = ns[g], fluid = 1.f - s;
    for (int i = 0; i < 19; ++i) {
        float ciu = cx[i] * ux + cy[i] * uy + cz[i] * uz;
        float feq = wt[i] * rho * (1.f + 3.f * ciu + 4.5f * ciu * ciu - 1.5f * uSqr);
        float bgk = fi[i] - omega * (fi[i] - feq);
        f[(long)i * n + g] = fluid * bgk + s * fi[opp[i]];
    }
}

// Stream + boundary conditions: outlet copy / free-stream Dirichlet / interior pull.
kernel void stream_bc(global const float* src, global float* dst, global const float* ns,
                      const float uIn, const int nx, const int ny, const int nz, const long n) {
    long g = get_global_id(0);
    if (g >= n) return;
    long z = g % nz, y = (g / nz) % ny, x = g / ((long)ny * nz);
    if (x == nx - 1) {                              // outlet: zero-gradient copy of upstream plane
        long up = (((long)(nx - 2) * ny + y) * nz + z);
        for (int i = 0; i < 19; ++i) dst[(long)i * n + g] = src[(long)i * n + up];
        return;
    }
    if (x == 0 || y == 0 || y == ny - 1 || z == 0 || z == nz - 1) {
        if (ns[g] > 0.5f) {                         // solid on a face keeps its collided value
            for (int i = 0; i < 19; ++i) dst[(long)i * n + g] = src[(long)i * n + g];
        } else {                                    // free-stream feq(rho=1, u=(uIn,0,0))
            float uSqr = uIn * uIn;
            for (int i = 0; i < 19; ++i) {
                float ciu = cx[i] * uIn;
                dst[(long)i * n + g] = wt[i] * (1.f + 3.f * ciu + 4.5f * ciu * ciu - 1.5f * uSqr);
            }
        }
        return;
    }
    for (int i = 0; i < 19; ++i) {                  // interior pull-stream
        long sx = x - cx[i], sy = y - cy[i], sz = z - cz[i];
        dst[(long)i * n + g] = src[(long)i * n + ((sx * ny + sy) * nz + sz)];
    }
}
)CL";

void clCheck(cl_int err, const char* what) {
    if (err != CL_SUCCESS)
        throw std::runtime_error(std::string("OpenCL error ") + std::to_string(err) + " at " + what);
}

// Pick the first GPU device (fall back to any device) of the first platform.
bool pickDevice(cl_platform_id& plat, cl_device_id& dev) {
    cl_uint nplat = 0;
    if (clGetPlatformIDs(0, nullptr, &nplat) != CL_SUCCESS || nplat == 0) return false;
    std::vector<cl_platform_id> plats(nplat);
    clGetPlatformIDs(nplat, plats.data(), nullptr);
    for (cl_platform_id p : plats) {
        cl_uint nd = 0;
        if (clGetDeviceIDs(p, CL_DEVICE_TYPE_GPU, 1, &dev, &nd) == CL_SUCCESS && nd > 0) {
            plat = p;
            return true;
        }
    }
    for (cl_platform_id p : plats) {
        cl_uint nd = 0;
        if (clGetDeviceIDs(p, CL_DEVICE_TYPE_ALL, 1, &dev, &nd) == CL_SUCCESS && nd > 0) {
            plat = p;
            return true;
        }
    }
    return false;
}

}  // namespace

struct WindTunnelOpenCL::Impl {
    std::int64_t nx, ny, nz, ncells;
    float omega, uIn;
    cl_context ctx = nullptr;
    cl_command_queue queue = nullptr;
    cl_program program = nullptr;
    cl_kernel collideK = nullptr, streamK = nullptr;
    cl_mem fA = nullptr, fB = nullptr, nsBuf = nullptr;
    cl_mem cur = nullptr, other = nullptr;
    std::vector<float> hf;   // host staging populations {q, ncells}
    std::vector<float> hns;  // host solid fraction {ncells}
    mutable std::vector<float> cache;
    mutable bool cacheValid = false;

    // RAII: release every OpenCL object here so a constructor that throws
    // mid-way (the unique_ptr<Impl> is already constructed) still cleans up.
    ~Impl() {
        if (collideK) clReleaseKernel(collideK);
        if (streamK) clReleaseKernel(streamK);
        if (program) clReleaseProgram(program);
        if (fA) clReleaseMemObject(fA);
        if (fB) clReleaseMemObject(fB);
        if (nsBuf) clReleaseMemObject(nsBuf);
        if (queue) clReleaseCommandQueue(queue);
        if (ctx) clReleaseContext(ctx);
    }

    std::int64_t idx(std::int64_t x, std::int64_t y, std::int64_t z) const {
        return (x * ny + y) * nz + z;
    }

    // Rest / free-stream equilibrium at u = (u,0,0), rho = 1 for population i.
    float feq(int i, float u) const {
        const float ciu = static_cast<float>(kCx[i]) * u;
        const float uSqr = u * u;
        return static_cast<float>(weight(i)) * (1.f + 3.f * ciu + 4.5f * ciu * ciu - 1.5f * uSqr);
    }

    void seedInitial() {
        hf.assign(static_cast<std::size_t>(19 * ncells), 0.f);
        hns.assign(static_cast<std::size_t>(ncells), 0.f);
        for (std::int64_t c = 0; c < ncells; ++c)
            for (int i = 0; i < 19; ++i)
                hf[static_cast<std::size_t>(i * ncells + c)] = feq(i, uIn);
    }

    void uploadState() {
        clCheck(clEnqueueWriteBuffer(queue, fA, CL_TRUE, 0,
                                     sizeof(float) * hf.size(), hf.data(), 0, nullptr, nullptr),
                "write fA");
        clCheck(clEnqueueWriteBuffer(queue, nsBuf, CL_TRUE, 0,
                                     sizeof(float) * hns.size(), hns.data(), 0, nullptr, nullptr),
                "write ns");
        cur = fA;
        other = fB;
        cacheValid = false;
    }

    void ensureDownloaded() const {
        if (cacheValid) return;
        cache.resize(static_cast<std::size_t>(19 * ncells));
        clCheck(clEnqueueReadBuffer(queue, cur, CL_TRUE, 0, sizeof(float) * cache.size(),
                                    cache.data(), 0, nullptr, nullptr),
                "read populations");
        cacheValid = true;
    }
};

WindTunnelOpenCL::WindTunnelOpenCL(std::int64_t nx, std::int64_t ny, std::int64_t nz, double omega,
                                   double uIn)
    : impl_(std::make_unique<Impl>()) {
    if (nx < 3 || ny < 3 || nz < 3)
        throw std::invalid_argument("WindTunnelOpenCL: each extent must be >= 3");
    auto& im = *impl_;
    im.nx = nx;
    im.ny = ny;
    im.nz = nz;
    im.ncells = nx * ny * nz;
    im.omega = static_cast<float>(omega);
    im.uIn = static_cast<float>(uIn);

    cl_platform_id plat;
    cl_device_id dev;
    if (!pickDevice(plat, dev)) throw std::runtime_error("WindTunnelOpenCL: no OpenCL device");

    cl_int err = CL_SUCCESS;
    im.ctx = clCreateContext(nullptr, 1, &dev, nullptr, nullptr, &err);
    clCheck(err, "clCreateContext");
    im.queue = clCreateCommandQueue(im.ctx, dev, 0, &err);
    clCheck(err, "clCreateCommandQueue");

    im.program = clCreateProgramWithSource(im.ctx, 1, &kKernelSource, nullptr, &err);
    clCheck(err, "clCreateProgramWithSource");
    if (clBuildProgram(im.program, 1, &dev, "", nullptr, nullptr) != CL_SUCCESS) {
        std::size_t len = 0;
        clGetProgramBuildInfo(im.program, dev, CL_PROGRAM_BUILD_LOG, 0, nullptr, &len);
        std::string log(len, '\0');
        clGetProgramBuildInfo(im.program, dev, CL_PROGRAM_BUILD_LOG, len, log.data(), nullptr);
        throw std::runtime_error("WindTunnelOpenCL kernel build failed:\n" + log);
    }
    im.collideK = clCreateKernel(im.program, "collide", &err);
    clCheck(err, "clCreateKernel collide");
    im.streamK = clCreateKernel(im.program, "stream_bc", &err);
    clCheck(err, "clCreateKernel stream_bc");

    const std::size_t bytes = sizeof(float) * static_cast<std::size_t>(19 * im.ncells);
    im.fA = clCreateBuffer(im.ctx, CL_MEM_READ_WRITE, bytes, nullptr, &err);
    clCheck(err, "buffer fA");
    im.fB = clCreateBuffer(im.ctx, CL_MEM_READ_WRITE, bytes, nullptr, &err);
    clCheck(err, "buffer fB");
    im.nsBuf = clCreateBuffer(im.ctx, CL_MEM_READ_WRITE,
                              sizeof(float) * static_cast<std::size_t>(im.ncells), nullptr, &err);
    clCheck(err, "buffer ns");

    im.seedInitial();
    im.uploadState();
}

// Cleanup lives in ~Impl (RAII), so destroying impl_ releases everything — both
// on normal destruction and on a constructor that threw part-way.
WindTunnelOpenCL::~WindTunnelOpenCL() = default;

void WindTunnelOpenCL::setObstacleSphere(double cx, double cy, double cz, double radius,
                                         bool sharp) {
    auto& im = *impl_;
    for (std::int64_t x = 0; x < im.nx; ++x)
        for (std::int64_t y = 0; y < im.ny; ++y)
            for (std::int64_t z = 0; z < im.nz; ++z) {
                const double dx = x - cx, dy = y - cy, dz = z - cz;
                const double phi = std::sqrt(dx * dx + dy * dy + dz * dz) - radius;
                const double ns =
                    sharp ? (phi < 0.0 ? 1.0 : 0.0) : geometry::sdfToSolidFraction(phi, 1.0);
                setNs(x, y, z, ns);
            }
    im.uploadState();
}

void WindTunnelOpenCL::setObstacleField(const geometry::VoxelField& obj, int ox, int oy, int oz,
                                        bool sharp) {
    auto& im = *impl_;
    for (int x = 0; x < obj.nx; ++x)
        for (int y = 0; y < obj.ny; ++y)
            for (int z = 0; z < obj.nz; ++z) {
                const std::int64_t tx = ox + x, ty = oy + y, tz = oz + z;
                if (tx < 0 || tx >= im.nx || ty < 0 || ty >= im.ny || tz < 0 || tz >= im.nz)
                    continue;
                const double phi = obj.distance[static_cast<std::size_t>(obj.index(x, y, z))];
                const double ns = sharp ? (phi < 0.0 ? 1.0 : 0.0)
                                        : geometry::sdfToSolidFraction(phi, obj.spacing);
                setNs(tx, ty, tz, ns);
            }
    im.uploadState();
}

// Write ns and, for solid cells, seed the populations at rest (so the solid
// stays quiescent under bounce-back) — mirrors WindTunnel3D::seedRest.
void WindTunnelOpenCL::setNs(std::int64_t x, std::int64_t y, std::int64_t z, double ns) {
    auto& im = *impl_;
    const std::int64_t c = im.idx(x, y, z);
    im.hns[static_cast<std::size_t>(c)] = static_cast<float>(ns);
    if (ns > 0.5)
        for (int i = 0; i < 19; ++i)
            im.hf[static_cast<std::size_t>(i * im.ncells + c)] = im.feq(i, 0.f);
}

void WindTunnelOpenCL::run(std::int64_t steps) {
    auto& im = *impl_;
    const std::size_t global = static_cast<std::size_t>(im.ncells);
    const cl_long n = static_cast<cl_long>(im.ncells);
    const cl_int nx = static_cast<cl_int>(im.nx), ny = static_cast<cl_int>(im.ny),
                 nz = static_cast<cl_int>(im.nz);
    // Static scalar args.
    clCheck(clSetKernelArg(im.collideK, 1, sizeof(cl_mem), &im.nsBuf), "arg collide ns");
    clCheck(clSetKernelArg(im.collideK, 2, sizeof(cl_long), &n), "arg collide n");
    clCheck(clSetKernelArg(im.collideK, 3, sizeof(cl_float), &im.omega), "arg collide omega");
    clCheck(clSetKernelArg(im.streamK, 2, sizeof(cl_mem), &im.nsBuf), "arg stream ns");
    clCheck(clSetKernelArg(im.streamK, 3, sizeof(cl_float), &im.uIn), "arg stream uIn");
    clCheck(clSetKernelArg(im.streamK, 4, sizeof(cl_int), &nx), "arg stream nx");
    clCheck(clSetKernelArg(im.streamK, 5, sizeof(cl_int), &ny), "arg stream ny");
    clCheck(clSetKernelArg(im.streamK, 6, sizeof(cl_int), &nz), "arg stream nz");
    clCheck(clSetKernelArg(im.streamK, 7, sizeof(cl_long), &n), "arg stream n");

    for (std::int64_t s = 0; s < steps; ++s) {
        clCheck(clSetKernelArg(im.collideK, 0, sizeof(cl_mem), &im.cur), "arg collide f");
        clCheck(clEnqueueNDRangeKernel(im.queue, im.collideK, 1, nullptr, &global, nullptr, 0,
                                       nullptr, nullptr),
                "enqueue collide");
        clCheck(clSetKernelArg(im.streamK, 0, sizeof(cl_mem), &im.cur), "arg stream src");
        clCheck(clSetKernelArg(im.streamK, 1, sizeof(cl_mem), &im.other), "arg stream dst");
        clCheck(clEnqueueNDRangeKernel(im.queue, im.streamK, 1, nullptr, &global, nullptr, 0,
                                       nullptr, nullptr),
                "enqueue stream");
        std::swap(im.cur, im.other);
    }
    clFinish(im.queue);
    im.cacheValid = false;
}

std::int64_t WindTunnelOpenCL::nx() const { return impl_->nx; }
std::int64_t WindTunnelOpenCL::ny() const { return impl_->ny; }
std::int64_t WindTunnelOpenCL::nz() const { return impl_->nz; }

std::array<double, 3> WindTunnelOpenCL::velocity(std::int64_t x, std::int64_t y,
                                                 std::int64_t z) const {
    const auto& im = *impl_;
    im.ensureDownloaded();
    const std::int64_t c = im.idx(x, y, z);
    double rho = 0, jx = 0, jy = 0, jz = 0;  // fp64 accumulation for the read-back
    for (int i = 0; i < 19; ++i) {
        const double v = im.cache[static_cast<std::size_t>(i * im.ncells + c)];
        rho += v;
        jx += v * kCx[i];
        jy += v * kCy[i];
        jz += v * kCz[i];
    }
    const double invRho = 1.0 / rho;
    return {jx * invRho, jy * invRho, jz * invRho};
}

double WindTunnelOpenCL::solidFraction(std::int64_t x, std::int64_t y, std::int64_t z) const {
    return impl_->hns[static_cast<std::size_t>(impl_->idx(x, y, z))];
}

std::vector<float> WindTunnelOpenCL::downloadPopulations() const {
    impl_->ensureDownloaded();
    return impl_->cache;
}

void WindTunnelOpenCL::writeVtk(const std::string& path) const {
    io::VtkStructuredWriter<3> w(impl_->nx, impl_->ny, impl_->nz);
    using Coord = io::VtkStructuredWriter<3>::Coord;
    w.addVector("velocity", [this](Coord c) { return velocity(c[0], c[1], c[2]); });
    w.addScalar("speed", [this](Coord c) {
        const auto u = velocity(c[0], c[1], c[2]);
        return std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    });
    w.addScalar("solid", [this](Coord c) { return solidFraction(c[0], c[1], c[2]); });
    w.write(path);
}

bool WindTunnelOpenCL::openclAvailable() {
    cl_platform_id plat;
    cl_device_id dev;
    return pickDevice(plat, dev);
}

}  // namespace cyberfluids::solver

#endif  // CYBERFLUIDS_WITH_OPENCL
