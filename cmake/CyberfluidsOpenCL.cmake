# Optional OpenCL GPU backend — a bespoke device-kernel wind-tunnel solver
# (real OpenCL C kernels), NOT the CPU-only forEachIndex seam. Builds only when
# CYBERFLUIDS_OPENCL is ON and an OpenCL runtime is found. Mirrors the Metal
# backend module.
set(CYBERFLUIDS_HAVE_OPENCL OFF CACHE INTERNAL "OpenCL backend built")

if(CYBERFLUIDS_OPENCL AND CYBERFLUIDS_HAVE_NUMPP)
    find_package(OpenCL QUIET)
    if(NOT OpenCL_FOUND)
        message(STATUS "Cyberfluids: CYBERFLUIDS_OPENCL is ON but no OpenCL runtime found — skipping")
        return()
    endif()

    add_library(cyberfluids_opencl STATIC src/opencl/wind_tunnel_opencl.cpp)
    target_include_directories(cyberfluids_opencl PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_compile_features(cyberfluids_opencl PUBLIC cxx_std_20)
    target_link_libraries(cyberfluids_opencl PUBLIC cyberfluids_core numpp::numpp
                          PRIVATE OpenCL::OpenCL)
    # Public define so consumers see the solver header; pin to OpenCL 1.2 (macOS
    # ceiling) and silence Apple's OpenCL-deprecation warnings.
    target_compile_definitions(cyberfluids_opencl
        PUBLIC CYBERFLUIDS_WITH_OPENCL
        PRIVATE CL_TARGET_OPENCL_VERSION=120 CL_SILENCE_DEPRECATION)

    set(CYBERFLUIDS_HAVE_OPENCL ON CACHE INTERNAL "OpenCL backend built")
    message(STATUS "Cyberfluids: OpenCL backend enabled (device-kernel wind tunnel)")
endif()
