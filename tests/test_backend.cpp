/// Verifies the CPU backend seam: forEachIndex visits every index in [0, n)
/// exactly once. Distinct indices are written, so this is race-free even on the
/// parallel path.

#include <cstdint>
#include <vector>

#include "cyberfluids/backend/backend.hpp"
#include "testing.hpp"

int main() {
    using B = cyberfluids::backend::Cpu;

    const std::int64_t n = 1000;
    std::vector<int> visits(static_cast<std::size_t>(n), 0);
    B::forEachIndex(n, [&](std::int64_t i) { visits[static_cast<std::size_t>(i)] += 1; });

    for (std::int64_t i = 0; i < n; ++i)
        CF_CHECK(visits[static_cast<std::size_t>(i)] == 1);

    std::printf("backend=%s parallel=%d\n", B::name, static_cast<int>(B::parallel));
    if (cftest::failures == 0) std::printf("backend: all checks passed\n");
    return cftest::failures;
}
