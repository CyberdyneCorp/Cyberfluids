#pragma once

#include <cstdint>

#include "cyberfluids/backend/counting_iterator.hpp"

// The parallel path is compiled only when the standard library advertises the
// parallel algorithms (`__cpp_lib_parallel_algorithm`). Otherwise we fall back
// to a serial loop, keeping the CPU backend fully functional everywhere
// (mobile, libc++ without a PSTL backend, …). See hardware-backends spec.
#if defined(__cpp_lib_parallel_algorithm) && !defined(CYBERFLUIDS_FORCE_SERIAL)
#    include <algorithm>
#    include <execution>
#    define CYBERFLUIDS_CPU_PARALLEL 1
#else
#    define CYBERFLUIDS_CPU_PARALLEL 0
#endif

namespace cyberfluids::backend {

/// CPU backend: runs a per-cell kernel across the index range `[0, n)`, using
/// `std::execution::par_unseq` when available and a serial loop otherwise.
/// The kernel `f(std::int64_t)` must be safe to call concurrently for distinct
/// indices (each cell writes only its own state).
struct Cpu {
    static constexpr const char* name = "cpu";
    static constexpr bool parallel = CYBERFLUIDS_CPU_PARALLEL != 0;

    template <class F>
    static void forEachIndex(std::int64_t n, F f) {
#if CYBERFLUIDS_CPU_PARALLEL
        std::for_each(std::execution::par_unseq, CountingIterator{0}, CountingIterator{n},
                      [f](std::int64_t i) { f(i); });
#else
        for (std::int64_t i = 0; i < n; ++i) f(i);
#endif
    }
};

}  // namespace cyberfluids::backend
