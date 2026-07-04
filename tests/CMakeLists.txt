# Dependency-free unit tests, run via CTest. Each test is a small executable
# that returns the number of failed checks (0 == pass).

add_executable(test_descriptors test_descriptors.cpp)
target_link_libraries(test_descriptors PRIVATE cyberfluids_core)
target_include_directories(test_descriptors PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME descriptors COMMAND test_descriptors)

# NumPP-backed tests build only when NumPP is available.
if(CYBERFLUIDS_HAVE_NUMPP)
    foreach(_t populations fields lattice)
        add_executable(test_${_t} test_${_t}.cpp)
        target_link_libraries(test_${_t} PRIVATE cyberfluids_core numpp::numpp)
        target_include_directories(test_${_t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        add_test(NAME ${_t} COMMAND test_${_t})
    endforeach()
endif()
