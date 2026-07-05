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
option(CYBERFLUIDS_BUILD_BENCH "Build the throughput benchmark (CPU/CUDA/OpenCL)" OFF)

# Build the C ABI as a self-contained shared library for the Python wheel
# (scikit-build-core). NumPP is statically linked in (PIC), so the packaged .so
# has no libnumpp runtime dependency — only system libtbb/libstdc++. Set by
# bindings/python/pyproject.toml; see the wheel install block below.
option(CYBERFLUIDS_PYTHON_WHEEL "Build + install the C ABI into the Python wheel" OFF)
if(CYBERFLUIDS_PYTHON_WHEEL)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(NUMPP_BUILD_SHARED OFF CACHE BOOL "" FORCE)  # absorb NumPP into libcyberfluids_c
endif()

# Backend feature flags. The CPU backend is the always-on, mobile-friendly
# baseline. GPU backends are OFF by default; absence of a GPU toolkit never
# breaks the CPU build. Cyberfluids is single-node and never depends on MPI.
option(CYBERFLUIDS_CPU    "Enable CPU backend (std::execution::par_unseq)" ON)
option(CYBERFLUIDS_CUDA   "Enable NVIDIA CUDA backend"                     OFF)
option(CYBERFLUIDS_METAL  "Enable Apple Metal backend (metal-cpp)"         OFF)
option(CYBERFLUIDS_OPENCL "Enable OpenCL backend"                          OFF)
option(CYBERFLUIDS_SYCL   "Enable SYCL backend"                            OFF)
option(CYBERFLUIDS_GEOMETRY "Enable STL/OBJ geometry import (CyberMeshGenerator)" OFF)

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
    # VERSION/SOVERSION give the shared object a semver-tracked SONAME so
    # consumers can pin ABI compatibility (major bumps == breaking).
    set_target_properties(cyberfluids_c PROPERTIES
        BUILD_RPATH "${CYBERFLUIDS_DEPS_PREFIX}/lib"
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}")

    # Python wheel: link libtbb into the .so so it carries a DT_NEEDED entry
    # (libstdc++ par_unseq needs it), and install the single self-contained
    # library into the `cyberfluids` package dir alongside __init__.py. NumPP is
    # statically linked (see CYBERFLUIDS_PYTHON_WHEEL above), so nothing else to
    # bundle. --no-as-needed pins libtbb regardless of link-line order.
    if(CYBERFLUIDS_PYTHON_WHEEL)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            target_link_options(cyberfluids_c PRIVATE "LINKER:--no-as-needed")
            target_link_libraries(cyberfluids_c PRIVATE tbb)
        endif()
        set_target_properties(cyberfluids_c PROPERTIES INSTALL_RPATH "$ORIGIN")
        # Own component so the wheel installs ONLY this library — the fetched
        # NumPP's (ungated) install rules stay out of the wheel (scikit-build-core
        # is told to install just the `python` component).
        install(TARGETS cyberfluids_c
            LIBRARY DESTINATION cyberfluids COMPONENT python
            RUNTIME DESTINATION cyberfluids COMPONENT python)
    endif()
endif()

# Optional Apple Metal GPU backend (defines cyberfluids_metal + CYBERFLUIDS_HAVE_METAL).
include(cmake/CyberfluidsMetal.cmake)

# Optional OpenCL GPU backend (defines cyberfluids_opencl + CYBERFLUIDS_HAVE_OPENCL).
include(cmake/CyberfluidsOpenCL.cmake)

# Optional CUDA GPU backend (defines cyberfluids_cuda + CYBERFLUIDS_HAVE_CUDA).
include(cmake/CyberfluidsCUDA.cmake)

# Optional STL/OBJ geometry import (defines cyberfluids_geometry + CYBERFLUIDS_HAVE_GEOMETRY).
include(cmake/CyberfluidsGeometry.cmake)

# When geometry is built, let the C ABI expose STL-loading wind-tunnel entry
# points (cf_wind_tunnel_create_from_stl). The tunnel itself is CMG-free and
# always present; only STL loading needs this link + define.
if(TARGET cyberfluids_c AND CYBERFLUIDS_HAVE_GEOMETRY)
    target_link_libraries(cyberfluids_c PRIVATE cyberfluids_geometry)
    target_compile_definitions(cyberfluids_c PRIVATE CYBERFLUIDS_WITH_GEOMETRY)
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

# Throughput benchmark (NumPP-backed; links any enabled GPU backends).
if(CYBERFLUIDS_BUILD_BENCH AND CYBERFLUIDS_HAVE_NUMPP)
    add_subdirectory(bench)
endif()

# -----------------------------------------------------------------------------
# Install + export (find_package(Cyberfluids))
#
# Produces an installable package with a CyberfluidsConfig.cmake so downstream
# projects can `find_package(Cyberfluids)` and link `Cyberfluids::core` /
# `Cyberfluids::c`. Requires an *installed* NumPP (the config re-resolves it via
# find_dependency), so it is disabled when NumPP came from the FetchContent
# fallback — that path is build-only, not redistributable.
# -----------------------------------------------------------------------------
option(CYBERFLUIDS_INSTALL "Generate install + find_package(Cyberfluids) rules" ON)

if(CYBERFLUIDS_INSTALL AND CYBERFLUIDS_NUMPP_FROM_PACKAGE)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Public headers (the whole tree is the API surface today).
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    # Export under clean names so the installed targets match the in-tree aliases
    # (Cyberfluids::core / Cyberfluids::c), not Cyberfluids::cyberfluids_core.
    set_target_properties(cyberfluids_core PROPERTIES EXPORT_NAME core)

    # Always export the header-only core; add the C ABI library when it was built.
    install(TARGETS cyberfluids_core EXPORT CyberfluidsTargets)
    if(TARGET cyberfluids_c)
        set_target_properties(cyberfluids_c PROPERTIES EXPORT_NAME c)
        install(TARGETS cyberfluids_c EXPORT CyberfluidsTargets
            RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()

    install(EXPORT CyberfluidsTargets
        FILE CyberfluidsTargets.cmake
        NAMESPACE Cyberfluids::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Cyberfluids)

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CyberfluidsConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/CyberfluidsConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Cyberfluids)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/CyberfluidsConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/CyberfluidsConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/CyberfluidsConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Cyberfluids)

    set(CYBERFLUIDS_INSTALL_ENABLED ON)
elseif(CYBERFLUIDS_INSTALL)
    set(CYBERFLUIDS_INSTALL_ENABLED OFF)
    message(STATUS "Cyberfluids: install/export skipped — NumPP is not an installed "
                   "package (FetchContent build). Install NumPP (scripts/bootstrap_deps.sh) "
                   "to produce a find_package(Cyberfluids) package.")
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
