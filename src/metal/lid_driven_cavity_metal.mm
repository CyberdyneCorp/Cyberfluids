// Objective-C++ implementation of the Metal D2Q9 BGK lid-driven cavity.
// Uses the system Metal framework (no external metal-cpp). Compiled with ARC.
#if defined(CYBERFLUIDS_WITH_METAL)

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>

#include "cyberfluids/solver/metal/lid_driven_cavity_metal.hpp"

namespace {

// Must match the MSL `CavityParams` struct exactly.
struct CavityParams {
    unsigned int nx;
    unsigned int ny;
    float omega;
    float lidU;
};

// D2Q9 (matches descriptors::D2Q9): index c = x*ny + y; f_i(c) = f[i*ncells + c].
// collide_bgk relaxes in place; stream_cavity is a pull-stream with no-slip walls
// and a moving-wall lid (faithful port of LidDrivenCavity2D::streamWithBoundaries).
const char* kKernelSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct CavityParams { uint nx; uint ny; float omega; float lidU; };

constant float w[9]  = {4.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/9.0,
                        1.0/36.0, 1.0/36.0, 1.0/36.0, 1.0/36.0};
constant int cx[9]   = {0, 1, 0, -1, 0, 1, -1, -1, 1};
constant int cy[9]   = {0, 0, 1, 0, -1, 1, 1, -1, -1};
constant int opp[9]  = {0, 3, 4, 1, 2, 7, 8, 5, 6};

kernel void collide_bgk(device float* f [[buffer(0)]],
                        constant CavityParams& p [[buffer(1)]],
                        uint gid [[thread_position_in_grid]]) {
    uint n = p.nx * p.ny;
    if (gid >= n) return;
    float fi[9];
    float rho = 0.0, jx = 0.0, jy = 0.0;
    for (int i = 0; i < 9; ++i) {
        fi[i] = f[i * n + gid];
        rho += fi[i];
        jx += fi[i] * cx[i];
        jy += fi[i] * cy[i];
    }
    float invRho = 1.0 / rho;
    float ux = jx * invRho, uy = jy * invRho;
    float uSqr = ux * ux + uy * uy;
    for (int i = 0; i < 9; ++i) {
        float ciu = cx[i] * ux + cy[i] * uy;
        float feq = w[i] * rho * (1.0 + 3.0 * ciu + 4.5 * ciu * ciu - 1.5 * uSqr);
        f[i * n + gid] = fi[i] - p.omega * (fi[i] - feq);
    }
}

kernel void stream_cavity(device const float* src [[buffer(0)]],
                          device float* dst [[buffer(1)]],
                          constant CavityParams& p [[buffer(2)]],
                          uint gid [[thread_position_in_grid]]) {
    uint n = p.nx * p.ny;
    if (gid >= n) return;
    int nx = int(p.nx), ny = int(p.ny);
    int y = int(gid) % ny;
    int x = int(gid) / ny;
    bool topInterior = (y == ny - 1) && x > 0 && x < nx - 1;
    for (int i = 0; i < 9; ++i) {
        int sx = x - cx[i];
        int sy = y - cy[i];
        if (sx >= 0 && sx < nx && sy >= 0 && sy < ny) {
            dst[i * n + gid] = src[i * n + uint(sx * ny + sy)];
        } else if (topInterior && cy[i] == -1) {
            // moving-wall bounce-back: noSlip + 2 w_i rho0 invCs2 (c_i . uWall), rho0=1, invCs2=3
            dst[i * n + gid] = src[opp[i] * n + gid] + 2.0 * w[i] * 3.0 * (cx[i] * p.lidU);
        } else {
            dst[i * n + gid] = src[opp[i] * n + gid];  // no-slip wall
        }
    }
}
)MSL";

const float kWeights[9] = {4.0f / 9,  1.0f / 9,  1.0f / 9,  1.0f / 9, 1.0f / 9,
                           1.0f / 36, 1.0f / 36, 1.0f / 36, 1.0f / 36};
const int kCx[9] = {0, 1, 0, -1, 0, 1, -1, -1, 1};
const int kCy[9] = {0, 0, 1, 0, -1, 1, 1, -1, -1};

}  // namespace

namespace cyberfluids::solver {

struct MetalLidDrivenCavity2D::Impl {
    std::int64_t nx, ny, ncells;
    CavityParams params;
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    id<MTLComputePipelineState> collidePipe;
    id<MTLComputePipelineState> streamPipe;
    id<MTLBuffer> popSrc;
    id<MTLBuffer> popDst;

    const float* pop() const { return static_cast<const float*>(popSrc.contents); }
};

MetalLidDrivenCavity2D::MetalLidDrivenCavity2D(std::int64_t nx, std::int64_t ny, double omega,
                                               double lidVelocity)
    : impl_(std::make_unique<Impl>()) {
    impl_->nx = nx;
    impl_->ny = ny;
    impl_->ncells = nx * ny;
    impl_->params = {static_cast<unsigned>(nx), static_cast<unsigned>(ny),
                     static_cast<float>(omega), static_cast<float>(lidVelocity)};

    impl_->device = MTLCreateSystemDefaultDevice();
    if (impl_->device == nil) throw std::runtime_error("MetalLidDrivenCavity2D: no Metal device");
    impl_->queue = [impl_->device newCommandQueue];

    NSError* err = nil;
    id<MTLLibrary> lib = [impl_->device newLibraryWithSource:@(kKernelSource)
                                                      options:nil
                                                        error:&err];
    if (lib == nil) {
        throw std::runtime_error(std::string("Metal kernel compile failed: ") +
                                 (err ? err.localizedDescription.UTF8String : "unknown"));
    }
    id<MTLFunction> fCollide = [lib newFunctionWithName:@"collide_bgk"];
    id<MTLFunction> fStream = [lib newFunctionWithName:@"stream_cavity"];
    impl_->collidePipe = [impl_->device newComputePipelineStateWithFunction:fCollide error:&err];
    impl_->streamPipe = [impl_->device newComputePipelineStateWithFunction:fStream error:&err];
    if (impl_->collidePipe == nil || impl_->streamPipe == nil)
        throw std::runtime_error("Metal pipeline creation failed");

    const NSUInteger bytes = sizeof(float) * 9 * static_cast<NSUInteger>(impl_->ncells);
    impl_->popSrc = [impl_->device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
    impl_->popDst = [impl_->device newBufferWithLength:bytes options:MTLResourceStorageModeShared];

    // Initialize at equilibrium (rho=1, u=0): f_i = t_i.
    float* p = static_cast<float*>(impl_->popSrc.contents);
    for (std::int64_t c = 0; c < impl_->ncells; ++c)
        for (int i = 0; i < 9; ++i) p[i * impl_->ncells + c] = kWeights[i];
}

MetalLidDrivenCavity2D::~MetalLidDrivenCavity2D() = default;

void MetalLidDrivenCavity2D::step() {
    const NSUInteger n = static_cast<NSUInteger>(impl_->ncells);
    NSUInteger tpg = std::min<NSUInteger>(impl_->collidePipe.maxTotalThreadsPerThreadgroup, 256);
    MTLSize grid = MTLSizeMake(n, 1, 1);
    MTLSize tg = MTLSizeMake(tpg, 1, 1);

    id<MTLCommandBuffer> cb = [impl_->queue commandBuffer];

    id<MTLComputeCommandEncoder> e1 = [cb computeCommandEncoder];
    [e1 setComputePipelineState:impl_->collidePipe];
    [e1 setBuffer:impl_->popSrc offset:0 atIndex:0];
    [e1 setBytes:&impl_->params length:sizeof(CavityParams) atIndex:1];
    [e1 dispatchThreads:grid threadsPerThreadgroup:tg];
    [e1 endEncoding];

    id<MTLComputeCommandEncoder> e2 = [cb computeCommandEncoder];
    [e2 setComputePipelineState:impl_->streamPipe];
    [e2 setBuffer:impl_->popSrc offset:0 atIndex:0];
    [e2 setBuffer:impl_->popDst offset:0 atIndex:1];
    [e2 setBytes:&impl_->params length:sizeof(CavityParams) atIndex:2];
    [e2 dispatchThreads:grid threadsPerThreadgroup:tg];
    [e2 endEncoding];

    [cb commit];
    [cb waitUntilCompleted];

    std::swap(impl_->popSrc, impl_->popDst);
}

void MetalLidDrivenCavity2D::run(std::int64_t steps) {
    for (std::int64_t s = 0; s < steps; ++s) step();
}

std::int64_t MetalLidDrivenCavity2D::nx() const { return impl_->nx; }
std::int64_t MetalLidDrivenCavity2D::ny() const { return impl_->ny; }

double MetalLidDrivenCavity2D::density(std::int64_t x, std::int64_t y) const {
    const float* f = impl_->pop();
    const std::int64_t c = x * impl_->ny + y, n = impl_->ncells;
    double rho = 0.0;
    for (int i = 0; i < 9; ++i) rho += f[i * n + c];
    return rho;
}

std::array<double, 2> MetalLidDrivenCavity2D::velocity(std::int64_t x, std::int64_t y) const {
    const float* f = impl_->pop();
    const std::int64_t c = x * impl_->ny + y, n = impl_->ncells;
    double rho = 0.0, jx = 0.0, jy = 0.0;
    for (int i = 0; i < 9; ++i) {
        rho += f[i * n + c];
        jx += f[i * n + c] * kCx[i];
        jy += f[i * n + c] * kCy[i];
    }
    return {jx / rho, jy / rho};
}

std::vector<float> MetalLidDrivenCavity2D::downloadPopulations() const {
    const float* f = impl_->pop();
    return std::vector<float>(f, f + 9 * impl_->ncells);
}

bool MetalLidDrivenCavity2D::metalAvailable() {
    id<MTLDevice> d = MTLCreateSystemDefaultDevice();
    return d != nil;
}

}  // namespace cyberfluids::solver

#endif  // CYBERFLUIDS_WITH_METAL
