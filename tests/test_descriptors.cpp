/// Validates the lattice descriptors against the LBM physical invariants a
/// DdQq stencil must satisfy. These double as regression tests: any change to a
/// velocity set or weight that breaks isotropy or the sound speed fails here.

#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "testing.hpp"

using cyberfluids::LatticeDescriptor;
using cyberfluids::descriptors::D2Q9;
using cyberfluids::descriptors::D3Q19;

namespace {

// A type that is NOT a valid descriptor (missing q/numPop/velocities/...).
struct NotADescriptor {
    static constexpr int d = 2;
};

// The Concept must accept the real descriptors and reject a non-conforming type.
static_assert(LatticeDescriptor<D2Q9>);
static_assert(LatticeDescriptor<D3Q19>);
static_assert(!LatticeDescriptor<NotADescriptor>);
static_assert(!LatticeDescriptor<int>);

template <LatticeDescriptor D>
void check_descriptor() {
    constexpr double tol = 1e-12;

    // numPop == q.
    CF_CHECK(D::numPop == D::q);

    // Weights sum to one.
    double sum_w = 0.0;
    for (int i = 0; i < D::q; ++i) sum_w += D::t[i];
    CF_CHECK_CLOSE(sum_w, 1.0, tol);

    // First moment of the weights vanishes: sum_i t_i c_i = 0.
    for (int a = 0; a < D::d; ++a) {
        double m1 = 0.0;
        for (int i = 0; i < D::q; ++i) m1 += D::t[i] * D::c[i][a];
        CF_CHECK_CLOSE(m1, 0.0, tol);
    }

    // Second moment: sum_i t_i c_ia c_ib = cs2 * delta_ab.
    for (int a = 0; a < D::d; ++a) {
        for (int b = 0; b < D::d; ++b) {
            double m2 = 0.0;
            for (int i = 0; i < D::q; ++i)
                m2 += D::t[i] * D::c[i][a] * D::c[i][b];
            const double expected = (a == b) ? D::cs2 : 0.0;
            CF_CHECK_CLOSE(m2, expected, tol);
        }
    }

    // invCs2 is the reciprocal of cs2.
    CF_CHECK_CLOSE(D::cs2 * D::invCs2, 1.0, tol);

    // cNormSqr matches |c_i|^2.
    for (int i = 0; i < D::q; ++i) {
        int norm = 0;
        for (int a = 0; a < D::d; ++a) norm += D::c[i][a] * D::c[i][a];
        CF_CHECK(norm == D::cNormSqr[i]);
    }

    // opposite is an involution and c[opposite[i]] == -c[i].
    for (int i = 0; i < D::q; ++i) {
        const int opp = D::opposite[i];
        CF_CHECK(D::opposite[opp] == i);
        for (int a = 0; a < D::d; ++a)
            CF_CHECK(D::c[opp][a] == -D::c[i][a]);
    }
}

}  // namespace

int main() {
    check_descriptor<D2Q9>();
    check_descriptor<D3Q19>();

    if (cftest::failures == 0)
        std::printf("descriptors: all checks passed\n");
    return cftest::failures;
}
