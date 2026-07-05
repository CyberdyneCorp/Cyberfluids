// CUDA mirror of the OpenCL wind tunnel. The two __global__ kernels below are
// line-for-line translations of the OpenCL C kernels in
// src/opencl/wind_tunnel_opencl.cpp (validated on Apple GPU to ~1e-5 vs the fp64
// CPU oracle). Authored on a machine without nvcc; compile + validate on an
// NVIDIA box with -DCYBERFLUIDS_CUDA=ON and the ctest `cuda_wind_tunnel`.

#if defined(CYBERFLUIDS_WITH_CUDA)

#include "cyberfluids/solver/cuda/wind_tunnel_cuda.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <cuda_runtime.h>

#include "cyberfluids/io/vtk_writer.hpp"

namespace cyberfluids::solver {

namespace {

constexpr int kCx[19] = {0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0};
constexpr int kCy[19] = {0, 0, 0, 1, -1, 0, 0, 1, -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, -1};
constexpr int kCz[19] = {0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, -1, 1, 1, -1, -1, 1};
inline double weight(int i) {
    return i == 0 ? 1.0 / 3.0 : (i < 7 ? 1.0 / 18.0 : 1.0 / 36.0);
}

void cudaCheck(cudaError_t e, const char* what) {
    if (e != cudaSuccess)
        throw std::runtime_error(std::string("CUDA error at ") + what + ": " +
                                 cudaGetErrorString(e));
}

// Device stencil constants (match include/cyberfluids/core/descriptors.hpp).
__constant__ int dCx[19] = {0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0};
__constant__ int dCy[19] = {0, 0, 0, 1, -1, 0, 0, 1, -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, -1};
__constant__ int dCz[19] = {0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, -1, -1, 1, 1, -1, -1, 1};
__constant__ int dOpp[19] = {0, 2, 1, 4, 3, 6, 5, 8, 7, 10, 9, 12, 11, 14, 13, 16, 15, 18, 17};
__constant__ float dWt[19] = {0.333333333f,
    0.055555556f, 0.055555556f, 0.055555556f, 0.055555556f, 0.055555556f, 0.055555556f,
    0.027777778f, 0.027777778f, 0.027777778f, 0.027777778f, 0.027777778f, 0.027777778f,
    0.027777778f, 0.027777778f, 0.027777778f, 0.027777778f, 0.027777778f, 0.027777778f};

}  // namespace

// Porous convex-blend collide, in place (force = 0).
__global__ void collideKernel(float* f, const float* ns, long long n, float omega) {
    long long g = (long long)blockIdx.x * blockDim.x + threadIdx.x;
    if (g >= n) return;
    float fi[19];
    float rho = 0.f, jx = 0.f, jy = 0.f, jz = 0.f;
    for (int i = 0; i < 19; ++i) {
        float v = f[(long long)i * n + g];
        fi[i] = v; rho += v;
        jx += v * dCx[i]; jy += v * dCy[i]; jz += v * dCz[i];
    }
    float invRho = 1.f / rho;
    float ux = jx * invRho, uy = jy * invRho, uz = jz * invRho;
    float uSqr = ux * ux + uy * uy + uz * uz;
    float s = ns[g], fluid = 1.f - s;
    for (int i = 0; i < 19; ++i) {
        float ciu = dCx[i] * ux + dCy[i] * uy + dCz[i] * uz;
        float feq = dWt[i] * rho * (1.f + 3.f * ciu + 4.5f * ciu * ciu - 1.5f * uSqr);
        float bgk = fi[i] - omega * (fi[i] - feq);
        f[(long long)i * n + g] = fluid * bgk + s * fi[dOpp[i]];
    }
}

// Stream + boundary conditions: outlet copy / free-stream Dirichlet / interior pull.
__global__ void streamKernel(const float* src, float* dst, const float* ns, float uIn, int nx,
                             int ny, int nz, long long n) {
    long long g = (long long)blockIdx.x * blockDim.x + threadIdx.x;
    if (g >= n) return;
    long long z = g % nz, y = (g / nz) % ny, x = g / ((long long)ny * nz);
    if (x == nx - 1) {
        long long up = (((long long)(nx - 2) * ny + y) * nz + z);
        for (int i = 0; i < 19; ++i) dst[(long long)i * n + g] = src[(long long)i * n + up];
        return;
    }
    if (x == 0 || y == 0 || y == ny - 1 || z == 0 || z == nz - 1) {
        if (ns[g] > 0.5f) {
            for (int i = 0; i < 19; ++i) dst[(long long)i * n + g] = src[(long long)i * n + g];
        } else {
            float uSqr = uIn * uIn;
            for (int i = 0; i < 19; ++i) {
                float ciu = dCx[i] * uIn;
                dst[(long long)i * n + g] = dWt[i] * (1.f + 3.f * ciu + 4.5f * ciu * ciu - 1.5f * uSqr);
            }
        }
        return;
    }
    for (int i = 0; i < 19; ++i) {
        long long sx = x - dCx[i], sy = y - dCy[i], sz = z - dCz[i];
        dst[(long long)i * n + g] = src[(long long)i * n + ((sx * ny + sy) * nz + sz)];
    }
}

struct WindTunnelCuda::Impl {
    std::int64_t nx, ny, nz, ncells;
    float omega, uIn;
    float *fA = nullptr, *fB = nullptr, *nsBuf = nullptr;
    float *cur = nullptr, *other = nullptr;
    std::vector<float> hf, hns;
    mutable std::vector<float> cache;
    mutable bool cacheValid = false;

    std::int64_t idx(std::int64_t x, std::int64_t y, std::int64_t z) const {
        return (x * ny + y) * nz + z;
    }
    float feq(int i, float u) const {
        const float ciu = static_cast<float>(kCx[i]) * u;
        return static_cast<float>(weight(i)) * (1.f + 3.f * ciu + 4.5f * ciu * ciu - 1.5f * u * u);
    }
    void seedInitial() {
        hf.assign(static_cast<std::size_t>(19 * ncells), 0.f);
        hns.assign(static_cast<std::size_t>(ncells), 0.f);
        for (std::int64_t c = 0; c < ncells; ++c)
            for (int i = 0; i < 19; ++i) hf[static_cast<std::size_t>(i * ncells + c)] = feq(i, uIn);
    }
    void uploadState() {
        cudaCheck(cudaMemcpy(fA, hf.data(), sizeof(float) * hf.size(), cudaMemcpyHostToDevice),
                  "upload fA");
        cudaCheck(cudaMemcpy(nsBuf, hns.data(), sizeof(float) * hns.size(), cudaMemcpyHostToDevice),
                  "upload ns");
        cur = fA;
        other = fB;
        cacheValid = false;
    }
    void ensureDownloaded() const {
        if (cacheValid) return;
        cache.resize(static_cast<std::size_t>(19 * ncells));
        cudaCheck(cudaMemcpy(cache.data(), cur, sizeof(float) * cache.size(),
                             cudaMemcpyDeviceToHost),
                  "download populations");
        cacheValid = true;
    }
};

WindTunnelCuda::WindTunnelCuda(std::int64_t nx, std::int64_t ny, std::int64_t nz, double omega,
                               double uIn)
    : impl_(std::make_unique<Impl>()) {
    if (nx < 3 || ny < 3 || nz < 3)
        throw std::invalid_argument("WindTunnelCuda: each extent must be >= 3");
    auto& im = *impl_;
    im.nx = nx; im.ny = ny; im.nz = nz; im.ncells = nx * ny * nz;
    im.omega = static_cast<float>(omega);
    im.uIn = static_cast<float>(uIn);
    const std::size_t bytes = sizeof(float) * static_cast<std::size_t>(19 * im.ncells);
    cudaCheck(cudaMalloc(&im.fA, bytes), "malloc fA");
    cudaCheck(cudaMalloc(&im.fB, bytes), "malloc fB");
    cudaCheck(cudaMalloc(&im.nsBuf, sizeof(float) * static_cast<std::size_t>(im.ncells)),
              "malloc ns");
    im.seedInitial();
    im.uploadState();
}

WindTunnelCuda::~WindTunnelCuda() {
    auto& im = *impl_;
    if (im.fA) cudaFree(im.fA);
    if (im.fB) cudaFree(im.fB);
    if (im.nsBuf) cudaFree(im.nsBuf);
}

void WindTunnelCuda::setNs(std::int64_t x, std::int64_t y, std::int64_t z, double ns) {
    auto& im = *impl_;
    const std::int64_t c = im.idx(x, y, z);
    im.hns[static_cast<std::size_t>(c)] = static_cast<float>(ns);
    if (ns > 0.5)
        for (int i = 0; i < 19; ++i)
            im.hf[static_cast<std::size_t>(i * im.ncells + c)] = im.feq(i, 0.f);
}

void WindTunnelCuda::setObstacleSphere(double cx, double cy, double cz, double radius, bool sharp) {
    auto& im = *impl_;
    for (std::int64_t x = 0; x < im.nx; ++x)
        for (std::int64_t y = 0; y < im.ny; ++y)
            for (std::int64_t z = 0; z < im.nz; ++z) {
                const double dx = x - cx, dy = y - cy, dz = z - cz;
                const double phi = std::sqrt(dx * dx + dy * dy + dz * dz) - radius;
                setNs(x, y, z,
                      sharp ? (phi < 0.0 ? 1.0 : 0.0) : geometry::sdfToSolidFraction(phi, 1.0));
            }
    im.uploadState();
}

void WindTunnelCuda::setObstacleField(const geometry::VoxelField& obj, int ox, int oy, int oz,
                                      bool sharp) {
    auto& im = *impl_;
    for (int x = 0; x < obj.nx; ++x)
        for (int y = 0; y < obj.ny; ++y)
            for (int z = 0; z < obj.nz; ++z) {
                const std::int64_t tx = ox + x, ty = oy + y, tz = oz + z;
                if (tx < 0 || tx >= im.nx || ty < 0 || ty >= im.ny || tz < 0 || tz >= im.nz)
                    continue;
                const double phi = obj.distance[static_cast<std::size_t>(obj.index(x, y, z))];
                setNs(tx, ty, tz,
                      sharp ? (phi < 0.0 ? 1.0 : 0.0) : geometry::sdfToSolidFraction(phi, obj.spacing));
            }
    im.uploadState();
}

void WindTunnelCuda::run(std::int64_t steps) {
    auto& im = *impl_;
    const int threads = 256;
    const int blocks = static_cast<int>((im.ncells + threads - 1) / threads);
    const long long n = static_cast<long long>(im.ncells);
    for (std::int64_t s = 0; s < steps; ++s) {
        collideKernel<<<blocks, threads>>>(im.cur, im.nsBuf, n, im.omega);
        streamKernel<<<blocks, threads>>>(im.cur, im.other, im.nsBuf, im.uIn,
                                          static_cast<int>(im.nx), static_cast<int>(im.ny),
                                          static_cast<int>(im.nz), n);
        std::swap(im.cur, im.other);
    }
    cudaCheck(cudaGetLastError(), "kernel launch");
    cudaCheck(cudaDeviceSynchronize(), "run sync");
    im.cacheValid = false;
}

std::int64_t WindTunnelCuda::nx() const { return impl_->nx; }
std::int64_t WindTunnelCuda::ny() const { return impl_->ny; }
std::int64_t WindTunnelCuda::nz() const { return impl_->nz; }

std::array<double, 3> WindTunnelCuda::velocity(std::int64_t x, std::int64_t y,
                                               std::int64_t z) const {
    const auto& im = *impl_;
    im.ensureDownloaded();
    const std::int64_t c = im.idx(x, y, z);
    double rho = 0, jx = 0, jy = 0, jz = 0;
    for (int i = 0; i < 19; ++i) {
        const double v = im.cache[static_cast<std::size_t>(i * im.ncells + c)];
        rho += v;
        jx += v * kCx[i]; jy += v * kCy[i]; jz += v * kCz[i];
    }
    const double invRho = 1.0 / rho;
    return {jx * invRho, jy * invRho, jz * invRho};
}

double WindTunnelCuda::solidFraction(std::int64_t x, std::int64_t y, std::int64_t z) const {
    return impl_->hns[static_cast<std::size_t>(impl_->idx(x, y, z))];
}

std::vector<float> WindTunnelCuda::downloadPopulations() const {
    impl_->ensureDownloaded();
    return impl_->cache;
}

void WindTunnelCuda::writeVtk(const std::string& path) const {
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

bool WindTunnelCuda::cudaAvailable() {
    int count = 0;
    return cudaGetDeviceCount(&count) == cudaSuccess && count > 0;
}

}  // namespace cyberfluids::solver

#endif  // CYBERFLUIDS_WITH_CUDA
