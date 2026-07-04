/// Checkpoint/restart validation: a populations .npy round-trip is byte-exact,
/// a mismatched-grid load is rejected, and — the load-bearing test — a run that
/// checkpoints then continues is bit-for-bit identical to loading that checkpoint
/// fresh and continuing (so an interrupted run resumes exactly).

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/io/checkpoint.hpp"
#include "cyberfluids/solver/lid_driven_cavity.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::D2Q9;
using cyberfluids::descriptors::D3Q19;

namespace {
// Bit-exact comparison of two population fields.
template <class Pop>
bool identical(Pop& a, Pop& b) {
    if (a.ncells() != b.ncells()) return false;
    const std::int64_t count = static_cast<std::int64_t>(Pop::q) * a.ncells();
    return std::memcmp(a.array().template typed_data<double>(),
                       b.array().template typed_data<double>(),
                       static_cast<std::size_t>(count) * sizeof(double)) == 0;
}
}  // namespace

int main() {
    const std::string path = "ckpt_test.npy";

    // (1) Byte-exact round trip of a PopulationField.
    {
        cyberfluids::PopulationField<double, D2Q9> src(100), dst(100);
        for (int i = 0; i < 9; ++i)
            for (std::int64_t c = 0; c < 100; ++c) src(i, c) = i * 1000.0 + c + 0.5;
        cyberfluids::io::saveCheckpoint(path, src);
        cyberfluids::io::loadCheckpoint(path, dst);
        CF_CHECK(identical(src, dst));
    }

    // (2) Mismatched grid is rejected.
    {
        cyberfluids::PopulationField<double, D2Q9> wrong(64);
        bool threw = false;
        try {
            cyberfluids::io::loadCheckpoint(path, wrong);  // file holds ncells=100
        } catch (const std::runtime_error&) {
            threw = true;
        }
        CF_CHECK(threw);
    }

    // (3) Restart equivalence: continue-from-memory == reload-from-disk.
    {
        using Cav = cyberfluids::solver::LidDrivenCavity3D<>;
        const std::int64_t N = 16;
        const double omega = 1.0 / 0.6, U = 0.05;

        Cav cav1(N, N, N, omega, U);
        cav1.run(100);
        cyberfluids::io::saveCheckpoint(path, cav1.lattice().populations());
        cav1.run(50);  // continue to step 150

        Cav cav2(N, N, N, omega, U);
        cyberfluids::io::loadCheckpoint(path, cav2.lattice().populations());  // restore step 100
        cav2.run(50);  // continue to step 150

        CF_CHECK(identical(cav1.lattice().populations(), cav2.lattice().populations()));
    }

    std::remove(path.c_str());
    if (cftest::failures == 0) std::printf("checkpoint: all checks passed\n");
    return cftest::failures;
}
