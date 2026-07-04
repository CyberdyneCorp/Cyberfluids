#pragma once

/// Compile-time external-field opt-in for lattice descriptors. Dependency-free
/// (no NumPP), so base descriptors that include it stay lightweight. The actual
/// per-cell storage is ExternalField in core/external.hpp.
/// See openspec/specs/external-fields/spec.md.

namespace cyberfluids {

namespace ext {

/// A per-cell body force occupying `D` scalars.
template <int D>
struct Force {
    static constexpr int numScalars = D;
    static constexpr int forceBeginsAt = 0;
    static constexpr int sizeOfForce = D;
    static constexpr int velocityBeginsAt = 0;
    static constexpr int sizeOfVelocity = 0;
};

/// A per-cell advection velocity occupying `D` scalars.
template <int D>
struct Velocity {
    static constexpr int numScalars = D;
    static constexpr int velocityBeginsAt = 0;
    static constexpr int sizeOfVelocity = D;
    static constexpr int forceBeginsAt = 0;
    static constexpr int sizeOfForce = 0;
};

}  // namespace ext

/// Compose an external-field spec onto a base descriptor without modifying the
/// base. The variant inherits the base's compile-time stencil constants and
/// adds a nested `ExternalSpec`.
template <class Base, class Spec>
struct WithExternal : Base {
    using ExternalSpec = Spec;
};

/// Number of external scalars a descriptor declares (0 if it declares none).
template <class D>
constexpr int numExternalScalars() {
    if constexpr (requires { D::ExternalSpec::numScalars; })
        return D::ExternalSpec::numScalars;
    else
        return 0;
}

}  // namespace cyberfluids
