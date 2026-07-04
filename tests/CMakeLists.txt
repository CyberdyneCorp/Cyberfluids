# Dependency-free unit tests, run via CTest. Each test is a small executable
# that returns the number of failed checks (0 == pass).

foreach(_t descriptors backend vtk_writer)
    add_executable(test_${_t} test_${_t}.cpp)
    target_link_libraries(test_${_t} PRIVATE cyberfluids_core)
    target_include_directories(test_${_t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${_t} COMMAND test_${_t})
endforeach()

# NumPP-backed tests build only when NumPP is available.
if(CYBERFLUIDS_HAVE_NUMPP)
    foreach(_t populations fields lattice bgk streaming boundary cavity dynamics_m1 poiseuille
               external external_force advection_diffusion ad_coupling
               buoyancy thermal_conduction rayleigh_benard shan_chen shan_chen_laplace
               multi_component_shan_chen checkpoint)
        add_executable(test_${_t} test_${_t}.cpp)
        target_link_libraries(test_${_t} PRIVATE cyberfluids_core numpp::numpp)
        target_include_directories(test_${_t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        add_test(NAME ${_t} COMMAND test_${_t})
    endforeach()

    # Metal GPU cavity test — built only when the Metal backend is present.
    if(CYBERFLUIDS_HAVE_METAL)
        add_executable(test_metal_cavity test_metal_cavity.cpp)
        target_link_libraries(test_metal_cavity PRIVATE cyberfluids_core numpp::numpp cyberfluids_metal)
        target_include_directories(test_metal_cavity PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        add_test(NAME metal_cavity COMMAND test_metal_cavity)
    endif()

    # Palabos oracle regression — built only when the stored reference exists.
    set(_oracle_ref "${CMAKE_CURRENT_SOURCE_DIR}/oracle/cavity2d_palabos.csv")
    if(EXISTS "${_oracle_ref}")
        add_executable(test_oracle_cavity test_oracle_cavity.cpp)
        target_link_libraries(test_oracle_cavity PRIVATE cyberfluids_core numpp::numpp)
        target_include_directories(test_oracle_cavity PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_compile_definitions(test_oracle_cavity PRIVATE
            CYBERFLUIDS_ORACLE_DATA="${_oracle_ref}")
        add_test(NAME oracle_cavity COMMAND test_oracle_cavity)
    endif()

    set(_oracle_ref3d "${CMAKE_CURRENT_SOURCE_DIR}/oracle/cavity3d_palabos.csv")
    if(EXISTS "${_oracle_ref3d}")
        add_executable(test_oracle_cavity3d test_oracle_cavity3d.cpp)
        target_link_libraries(test_oracle_cavity3d PRIVATE cyberfluids_core numpp::numpp)
        target_include_directories(test_oracle_cavity3d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_compile_definitions(test_oracle_cavity3d PRIVATE
            CYBERFLUIDS_ORACLE_DATA3D="${_oracle_ref3d}")
        add_test(NAME oracle_cavity3d COMMAND test_oracle_cavity3d)
    endif()
endif()
