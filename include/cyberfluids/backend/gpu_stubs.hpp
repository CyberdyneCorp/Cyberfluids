#pragma once

#include <cstdint>

// GPU backend stubs. These lock the backend seam — every backend exposes the
// same static `forEachIndex(n, f)` contract as Cpu — but no device kernels are
// implemented yet (that is a follow-up change per capability hardware-backends).
// Each stub is compiled only when its feature flag is set by CMake
// (-DCYBERFLUIDS_WITH_CUDA, _METAL, _OPENCL, _SYCL). Until real kernels land
// they execute the kernel on the host so a flag-enabled build still runs.

namespace cyberfluids::backend {

#if defined(CYBERFLUIDS_WITH_CUDA)
struct Cuda {
    static constexpr const char* name = "cuda";
    template <class F>
    static void forEachIndex(std::int64_t n, F f) {
        for (std::int64_t i = 0; i < n; ++i) f(i);  // TODO: launch a CUDA kernel
    }
};
#endif

#if defined(CYBERFLUIDS_WITH_METAL)
struct Metal {
    static constexpr const char* name = "metal";
    template <class F>
    static void forEachIndex(std::int64_t n, F f) {
        for (std::int64_t i = 0; i < n; ++i) f(i);  // TODO: dispatch a Metal compute kernel
    }
};
#endif

#if defined(CYBERFLUIDS_WITH_OPENCL)
struct OpenCL {
    static constexpr const char* name = "opencl";
    template <class F>
    static void forEachIndex(std::int64_t n, F f) {
        for (std::int64_t i = 0; i < n; ++i) f(i);  // TODO: enqueue an OpenCL kernel
    }
};
#endif

#if defined(CYBERFLUIDS_WITH_SYCL)
struct Sycl {
    static constexpr const char* name = "sycl";
    template <class F>
    static void forEachIndex(std::int64_t n, F f) {
        for (std::int64_t i = 0; i < n; ++i) f(i);  // TODO: submit a SYCL parallel_for
    }
};
#endif

}  // namespace cyberfluids::backend
