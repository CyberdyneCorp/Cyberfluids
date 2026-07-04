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

list(PREPEND CMAKE_PREFIX_PATH "${CYBERFLUIDS_DEPS_PREFIX}")

find_package(NumPP CONFIG QUIET
    NO_CMAKE_PACKAGE_REGISTRY
    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
if(NumPP_FOUND)
    set(CYBERFLUIDS_HAVE_NUMPP ON)
    message(STATUS "Cyberfluids: found NumPP ${NumPP_VERSION}")
else()
    message(WARNING
        "Cyberfluids: NumPP not found under '${CYBERFLUIDS_DEPS_PREFIX}'. "
        "Header-only descriptor targets still build. "
        "Run scripts/bootstrap_deps.sh to install NumPP/SciPP and enable "
        "NumPP-backed features.")
endif()

find_package(SciPP CONFIG QUIET
    NO_CMAKE_PACKAGE_REGISTRY
    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
if(SciPP_FOUND)
    set(CYBERFLUIDS_HAVE_SCIPP ON)
    message(STATUS "Cyberfluids: found SciPP ${SciPP_VERSION}")
endif()
