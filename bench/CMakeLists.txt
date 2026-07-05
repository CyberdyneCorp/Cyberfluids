# Throughput benchmark (not a test — no pass/fail, just GLUPS numbers). Built
# only when NumPP is available. Links whichever GPU backends were enabled so the
# same binary reports CPU + CUDA + OpenCL side by side; a CPU-only host still
# gets a CPU number. Run it with `just bench` or directly.
add_executable(wind_tunnel_bench wind_tunnel_bench.cpp)
target_link_libraries(wind_tunnel_bench PRIVATE cyberfluids_core numpp::numpp)
target_include_directories(wind_tunnel_bench PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(CYBERFLUIDS_HAVE_CUDA)
    target_link_libraries(wind_tunnel_bench PRIVATE cyberfluids_cuda)
    target_compile_definitions(wind_tunnel_bench PRIVATE CYBERFLUIDS_WITH_CUDA)
endif()

if(CYBERFLUIDS_HAVE_OPENCL)
    target_link_libraries(wind_tunnel_bench PRIVATE cyberfluids_opencl)
    target_compile_definitions(wind_tunnel_bench PRIVATE CYBERFLUIDS_WITH_OPENCL)
endif()
