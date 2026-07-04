/// Verifies BlockLattice: bounding box, cell views aliasing the SoA storage,
/// per-region dynamics attribution, and smart-pointer ownership (no manual
/// delete — lattices live in unique_ptr, dynamics in the lattice's registry).

#include <cstdint>
#include <memory>

#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/dynamics.hpp"
#include "cyberfluids/core/geometry.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "testing.hpp"

using cyberfluids::descriptors::D2Q9;
using cyberfluids::descriptors::D3Q19;

using Cell2 = cyberfluids::Cell<double, D2Q9>;
using Dyn2 = cyberfluids::Dynamics<double, D2Q9>;

namespace {

// Minimal concrete Dynamics used only to exercise attribution/lookup.
struct StubDynamics : Dyn2 {
    double omega_ = 1.5;
    void collide(Cell2&) const override {}
    double computeEquilibrium(int, double, const Velocity&, double) const override {
        return 0.0;
    }
    double computeDensity(const Cell2&) const override { return 0.0; }
    void computeVelocity(const Cell2&, Velocity&) const override {}
    double getOmega() const override { return omega_; }
    void setOmega(double o) override { omega_ = o; }
};

}  // namespace

int main() {
    using Lattice = cyberfluids::BlockLattice2D<double, D2Q9>;
    auto lat = std::make_unique<Lattice>(4, 3);

    // Bounding box and extent.
    const auto box = lat->getBoundingBox();
    CF_CHECK(box.lo[0] == 0 && box.hi[0] == 3);
    CF_CHECK(box.lo[1] == 0 && box.hi[1] == 2);
    CF_CHECK(box.size() == 12);
    CF_CHECK(lat->ncells() == 12);

    // A cell view aliases the underlying SoA populations.
    lat->get(2, 1)[5] = 7.0;
    const std::int64_t c = lat->index(2, 1);
    CF_CHECK(lat->populations()(5, c) == 7.0);
    CF_CHECK(lat->get(2, 1)[5] == 7.0);

    // Dynamics attribution over a sub-box [1..2] x [0..1].
    auto stub = std::make_shared<StubDynamics>();
    cyberfluids::Box<2> sub;
    sub.lo[0] = 1;
    sub.lo[1] = 0;
    sub.hi[0] = 2;
    sub.hi[1] = 1;
    lat->attributeDynamics(sub, stub);

    CF_CHECK(lat->get(1, 0).dynamics() == stub.get());
    CF_CHECK(lat->get(2, 1).dynamics() == stub.get());
    CF_CHECK(lat->get(0, 0).dynamics() == nullptr);  // outside the sub-box
    CF_CHECK(lat->get(3, 2).dynamics() == nullptr);
    CF_CHECK(lat->get(1, 0).dynamics()->getOmega() == 1.5);

    // 3D smoke test.
    using Lattice3 = cyberfluids::BlockLattice3D<double, D3Q19>;
    auto lat3 = std::make_unique<Lattice3>(2, 2, 2);
    CF_CHECK(lat3->ncells() == 8);
    lat3->get(1, 1, 1)[18] = 3.0;
    CF_CHECK(lat3->populations()(18, lat3->index(1, 1, 1)) == 3.0);

    if (cftest::failures == 0) std::printf("lattice: all checks passed\n");
    return cftest::failures;  // unique_ptr/shared_ptr free everything — no delete
}
