# -----------------------------------------------------------------------------
# NumPP / SciPP resolution
#
# SciPP itself consumes NumPP via find_package(NumPP CONFIG) from an install
# prefix, so Cyberfluids uses the same pattern: build + install NumPP and SciPP
# into a local prefix (scripts/bootstrap_deps.sh), then find_package here.
#
# The header-only descriptor/Concept layer needs neither library, so a missing
# dependency is a warning (not fatal): those targets still build and test. The
# NumPP-backed data structures (task group 3) require NumPP.
# -----------------------------------------------------------------------------
set(CYBERFLUIDS_DEPS_PREFIX "${CMAKE_SOURCE_DIR}/.deps"
    CACHE PATH "Install prefix where NumPP/SciPP were installed")

# This module is include()d, so it shares the caller's scope — a plain set()
# is visible to the top-level CMakeLists (no PARENT_SCOPE needed).
set(CYBERFLUIDS_HAVE_NUMPP OFF)
set(CYBERFLUIDS_HAVE_SCIPP OFF)
# Tracks whether NumPP came from an installed CONFIG package (find_package) rather
# than an in-tree FetchContent build. Only the former is exportable, so the
# install/find_package(Cyberfluids) rules key off this.
set(CYBERFLUIDS_NUMPP_FROM_PACKAGE OFF)

# When NumPP is not already installed under the prefix, fetch + build it in-tree
# so a plain `cmake -B build` works with no manual sibling checkout. Pin the tag
# for reproducibility; override with -DCYBERFLUIDS_NUMPP_TAG=... . Turn the whole
# fallback off with -DCYBERFLUIDS_FETCH_DEPS=OFF (e.g. offline / vendored builds).
option(CYBERFLUIDS_FETCH_DEPS "Fetch NumPP via FetchContent when not installed" ON)
set(CYBERFLUIDS_NUMPP_TAG "v1.6.0" CACHE STRING "NumPP git tag for the FetchContent fallback")
set(CYBERFLUIDS_NUMPP_REPO "https://github.com/CyberdyneCorp/NumPP.git"
    CACHE STRING "NumPP git repository for the FetchContent fallback")

list(PREPEND CMAKE_PREFIX_PATH "${CYBERFLUIDS_DEPS_PREFIX}")

# The Python wheel needs NumPP statically absorbed into libcyberfluids_c (a
# self-contained .so), so it must use the FetchContent static build — skip the
# installed (shared) package even when one is present under the deps prefix.
if(NOT CYBERFLUIDS_PYTHON_WHEEL)
    find_package(NumPP CONFIG QUIET
        NO_CMAKE_PACKAGE_REGISTRY
        NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
endif()
if(NumPP_FOUND)
    set(CYBERFLUIDS_HAVE_NUMPP ON)
    set(CYBERFLUIDS_NUMPP_FROM_PACKAGE ON)
    message(STATUS "Cyberfluids: found installed NumPP ${NumPP_VERSION}")
elseif(CYBERFLUIDS_FETCH_DEPS OR CYBERFLUIDS_PYTHON_WHEEL)
    message(STATUS "Cyberfluids: NumPP not installed under '${CYBERFLUIDS_DEPS_PREFIX}'; "
                   "fetching ${CYBERFLUIDS_NUMPP_TAG} via FetchContent")
    include(FetchContent)
    # Silence NumPP's own tests/examples/benchmarks in the fetched build.
    set(NUMPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(NUMPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(NUMPP_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(NumPP
        GIT_REPOSITORY "${CYBERFLUIDS_NUMPP_REPO}"
        GIT_TAG "${CYBERFLUIDS_NUMPP_TAG}"
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(NumPP)
    set(CYBERFLUIDS_HAVE_NUMPP ON)  # numpp::numpp target now exists in-tree
    message(STATUS "Cyberfluids: using FetchContent NumPP (build-only; install/export disabled)")
else()
    message(WARNING
        "Cyberfluids: NumPP not found under '${CYBERFLUIDS_DEPS_PREFIX}' and "
        "CYBERFLUIDS_FETCH_DEPS is OFF. Header-only descriptor targets still build. "
        "Run scripts/bootstrap_deps.sh to install NumPP, or enable the fetch fallback.")
endif()

find_package(SciPP CONFIG QUIET
    NO_CMAKE_PACKAGE_REGISTRY
    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
if(SciPP_FOUND)
    set(CYBERFLUIDS_HAVE_SCIPP ON)
    message(STATUS "Cyberfluids: found SciPP ${SciPP_VERSION}")
endif()
