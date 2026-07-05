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

/// `N` generic per-cell scalars (e.g. a porous solid fraction).
template <int N>
struct Scalar {
    static constexpr int numScalars = N;
    static constexpr int scalarBeginsAt = 0;
    static constexpr int sizeOfScalar = N;
    static constexpr int forceBeginsAt = 0;
    static constexpr int sizeOfForce = 0;
    static constexpr int velocityBeginsAt = 0;
    static constexpr int sizeOfVelocity = 0;
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

/// Size of the declared force block (0 if the descriptor declares no force).
/// Distinguishes a force spec from a velocity spec so a mismatched dynamics
/// fails to compile instead of silently reading the wrong slots.
template <class D>
constexpr int numForceScalars() {
    if constexpr (requires { D::ExternalSpec::sizeOfForce; })
        return D::ExternalSpec::sizeOfForce;
    else
        return 0;
}

/// Size of the declared advection-velocity block (0 if none).
template <class D>
constexpr int numVelocityScalars() {
    if constexpr (requires { D::ExternalSpec::sizeOfVelocity; })
        return D::ExternalSpec::sizeOfVelocity;
    else
        return 0;
}

/// Size of the declared generic-scalar block (0 if none). Used for the porous
/// solid-fraction slot; distinct from force/velocity so a mismatched dynamics
/// fails to compile rather than reading the wrong offset.
template <class D>
constexpr int numScalarScalars() {
    if constexpr (requires { D::ExternalSpec::sizeOfScalar; })
        return D::ExternalSpec::sizeOfScalar;
    else
        return 0;
}

}  // namespace cyberfluids
