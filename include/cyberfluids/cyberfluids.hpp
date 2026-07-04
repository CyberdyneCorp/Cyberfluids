#pragma once

/// Cyberfluids — zero-legacy C++20 Lattice Boltzmann CFD engine.
/// Umbrella header: a single include for the public API.
///
/// The NumPP-backed headers (populations, fields, lattice) require NumPP on the
/// include path; link `numpp::numpp`. The descriptor/Concept layer is
/// dependency-free.

#include "cyberfluids/core/descriptor.hpp"
#include "cyberfluids/core/descriptors.hpp"
#include "cyberfluids/core/geometry.hpp"
#include "cyberfluids/core/cell.hpp"
#include "cyberfluids/core/dynamics.hpp"
#include "cyberfluids/core/populations.hpp"
#include "cyberfluids/core/fields.hpp"
#include "cyberfluids/core/lattice.hpp"
#include "cyberfluids/dynamics/bgk.hpp"
#include "cyberfluids/dynamics/trt.hpp"
#include "cyberfluids/dynamics/mrt.hpp"
#include "cyberfluids/dynamics/regularized.hpp"
#include "cyberfluids/dynamics/forced.hpp"
