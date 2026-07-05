# Optional STL/OBJ geometry import via CyberMeshGenerator (CMG, MIT). Builds only
# when CYBERFLUIDS_GEOMETRY is ON. CMG is brought in with add_subdirectory (NOT
# find_package): it ships no install/export config, and its public target needs a
# header generated in its build tree, which only an in-tree configure produces.
set(CYBERFLUIDS_HAVE_GEOMETRY OFF CACHE INTERNAL "Geometry (CMG) component built")

# Where the CyberMeshGenerator checkout lives (sibling of this repo by default,
# matching the NumPP convention in scripts/bootstrap_deps.sh).
set(CYBERFLUIDS_CMG_DIR "${CMAKE_SOURCE_DIR}/../CyberMeshGenerator"
    CACHE PATH "Path to a CyberMeshGenerator source checkout")

if(CYBERFLUIDS_GEOMETRY AND CYBERFLUIDS_HAVE_NUMPP)
    if(NOT EXISTS "${CYBERFLUIDS_CMG_DIR}/CMakeLists.txt")
        message(WARNING "Cyberfluids: CYBERFLUIDS_GEOMETRY is ON but no CyberMeshGenerator at "
                        "'${CYBERFLUIDS_CMG_DIR}' — set CYBERFLUIDS_CMG_DIR. Skipping geometry.")
        return()
    endif()

    # Build only the CMG core library (no tests/CLI/shared/GPU).
    set(CMG_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
    set(CMG_BUILD_CLI    OFF CACHE BOOL "" FORCE)
    set(CMG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CYBERFLUIDS_CMG_DIR}" "${CMAKE_BINARY_DIR}/cmg" EXCLUDE_FROM_ALL)

    # The one translation unit that sees CMG headers; everything else stays CMG-free.
    add_library(cyberfluids_geometry STATIC src/geometry/cmg_loader.cpp)
    target_include_directories(cyberfluids_geometry PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_compile_features(cyberfluids_geometry PUBLIC cxx_std_20)
    target_link_libraries(cyberfluids_geometry PUBLIC cyberfluids_core
                          PRIVATE cmg::cmg numpp::numpp)
    target_compile_definitions(cyberfluids_geometry PUBLIC CYBERFLUIDS_WITH_GEOMETRY)

    set(CYBERFLUIDS_HAVE_GEOMETRY ON CACHE INTERNAL "Geometry (CMG) component built")
    message(STATUS "Cyberfluids: geometry import enabled (CyberMeshGenerator at ${CYBERFLUIDS_CMG_DIR})")
endif()
