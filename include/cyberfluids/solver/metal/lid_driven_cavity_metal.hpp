#pragma once

// Metal GPU solver for the 2D D2Q9 BGK lid-driven cavity. Available only on a
// Metal build (CYBERFLUIDS_WITH_METAL, set by cmake/CyberfluidsMetal.cmake on
// Apple). A pimpl keeps the Objective-C/Metal headers out of this C++ header.
// See openspec/specs/hardware-backends/spec.md.
#if defined(CYBERFLUIDS_WITH_METAL)

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace cyberfluids::solver {

/// GPU counterpart of LidDrivenCavity2D<> (fp32). Populations live in Metal
/// buffers; BGK collide + the cavity pull-stream run as MSL compute kernels.
class MetalLidDrivenCavity2D {
public:
    MetalLidDrivenCavity2D(std::int64_t nx, std::int64_t ny, double omega, double lidVelocity);
    ~MetalLidDrivenCavity2D();
    MetalLidDrivenCavity2D(const MetalLidDrivenCavity2D&) = delete;
    MetalLidDrivenCavity2D& operator=(const MetalLidDrivenCavity2D&) = delete;

    void step();
    void run(std::int64_t steps);

    std::int64_t nx() const;
    std::int64_t ny() const;

    double density(std::int64_t x, std::int64_t y) const;
    std::array<double, 2> velocity(std::int64_t x, std::int64_t y) const;

    /// Raw device populations {q, ncells} as fp32 (for determinism tests).
    std::vector<float> downloadPopulations() const;

    /// True if a Metal device is available (else the solver cannot be used).
    static bool metalAvailable();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cyberfluids::solver

#endif  // CYBERFLUIDS_WITH_METAL
