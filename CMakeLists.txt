cmake_minimum_required(VERSION 3.24)

project(Cyberfluids
    VERSION 0.0.1
    DESCRIPTION "Zero-legacy C++20 Lattice Boltzmann CFD engine"
    HOMEPAGE_URL "https://github.com/CyberdyneCorp/Cyberfluids"
    LANGUAGES CXX)

# -----------------------------------------------------------------------------
# Language standard
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
option(CYBERFLUIDS_BUILD_TESTS "Build the test suite" ON)
option(CYBERFLUIDS_BUILD_EXAMPLES "Build the example demos" ON)

# Backend feature flags. The CPU backend is the always-on, mobile-friendly
# baseline. GPU backends are OFF by default; absence of a GPU toolkit never
# breaks the CPU build. Cyberfluids is single-node and never depends on MPI.
option(CYBERFLUIDS_CPU    "Enable CPU backend (std::execution::par_unseq)" ON)
option(CYBERFLUIDS_CUDA   "Enable NVIDIA CUDA backend"                     OFF)
option(CYBERFLUIDS_METAL  "Enable Apple Metal backend (metal-cpp)"         OFF)
option(CYBERFLUIDS_OPENCL "Enable OpenCL backend"                          OFF)
option(CYBERFLUIDS_SYCL   "Enable SYCL backend"                            OFF)

# -----------------------------------------------------------------------------
# Dependencies (NumPP / SciPP — CyberdyneCorp scientific suite)
# -----------------------------------------------------------------------------
include(cmake/CyberfluidsDependencies.cmake)

# -----------------------------------------------------------------------------
# Core library
#
# Header-only today (compile-time descriptors, Concepts). It gains compiled
# translation units and links NumPP/SciPP as the NumPP-backed data structures
# land (see the bootstrap-cyberfluids-core change, task group 3).
# -----------------------------------------------------------------------------
add_library(cyberfluids_core INTERFACE)
add_library(Cyberfluids::core ALIAS cyberfluids_core)
target_include_directories(cyberfluids_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(cyberfluids_core INTERFACE cxx_std_20)

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
# C ABI shared library (wrapped by the Python and Swift bindings)
# -----------------------------------------------------------------------------
if(CYBERFLUIDS_HAVE_NUMPP)
    add_library(cyberfluids_c SHARED src/capi.cpp)
    add_library(Cyberfluids::c ALIAS cyberfluids_c)
    target_link_libraries(cyberfluids_c PRIVATE cyberfluids_core numpp::numpp)
    target_compile_definitions(cyberfluids_c PRIVATE CYBERFLUIDS_VERSION="${PROJECT_VERSION}")
    # Find libnumpp at runtime (build tree) when the bindings load this library.
    set_target_properties(cyberfluids_c PROPERTIES
        BUILD_RPATH "${CYBERFLUIDS_DEPS_PREFIX}/lib")
endif()

if(CYBERFLUIDS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    # Binding smoke tests wrap the C ABI library.
    if(CYBERFLUIDS_HAVE_NUMPP)
        add_subdirectory(bindings)
    endif()
endif()

# Examples use the NumPP-backed solver, so they need NumPP present.
if(CYBERFLUIDS_BUILD_EXAMPLES AND CYBERFLUIDS_HAVE_NUMPP)
    add_subdirectory(examples)
endif()

# -----------------------------------------------------------------------------
# Configuration summary
# -----------------------------------------------------------------------------
message(STATUS "-----------------------------------------------------------")
message(STATUS "Cyberfluids ${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
message(STATUS "  Backends: CPU=${CYBERFLUIDS_CPU} CUDA=${CYBERFLUIDS_CUDA} "
               "Metal=${CYBERFLUIDS_METAL} OpenCL=${CYBERFLUIDS_OPENCL} SYCL=${CYBERFLUIDS_SYCL}")
message(STATUS "  NumPP found: ${CYBERFLUIDS_HAVE_NUMPP}   SciPP found: ${CYBERFLUIDS_HAVE_SCIPP}")
message(STATUS "  Tests: ${CYBERFLUIDS_BUILD_TESTS}")
message(STATUS "-----------------------------------------------------------")
