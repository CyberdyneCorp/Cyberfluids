# Optional CUDA GPU backend — a bespoke device-kernel wind-tunnel solver, the
# NVIDIA mirror of the OpenCL path. Authored without nvcc; enable with
# -DCYBERFLUIDS_CUDA=ON on a machine with the CUDA toolkit to compile + validate.
set(CYBERFLUIDS_HAVE_CUDA OFF CACHE INTERNAL "CUDA backend built")

if(CYBERFLUIDS_CUDA AND CYBERFLUIDS_HAVE_NUMPP)
    include(CheckLanguage)
    check_language(CUDA)
    if(NOT CMAKE_CUDA_COMPILER)
        message(STATUS "Cyberfluids: CYBERFLUIDS_CUDA is ON but no CUDA compiler found — skipping")
        return()
    endif()
    enable_language(CUDA)

    add_library(cyberfluids_cuda STATIC src/cuda/wind_tunnel_cuda.cu)
    set_target_properties(cyberfluids_cuda PROPERTIES CUDA_STANDARD 20 CUDA_STANDARD_REQUIRED ON)
    target_include_directories(cyberfluids_cuda PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(cyberfluids_cuda PUBLIC cyberfluids_core numpp::numpp)
    target_compile_definitions(cyberfluids_cuda PUBLIC CYBERFLUIDS_WITH_CUDA)

    set(CYBERFLUIDS_HAVE_CUDA ON CACHE INTERNAL "CUDA backend built")
    message(STATUS "Cyberfluids: CUDA backend enabled (device-kernel wind tunnel)")
endif()
