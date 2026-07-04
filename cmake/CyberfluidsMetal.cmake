# Apple Metal GPU backend (Objective-C++ + the system Metal framework; no
# external metal-cpp). Builds only on Apple when CYBERFLUIDS_METAL is ON.
set(CYBERFLUIDS_HAVE_METAL OFF CACHE INTERNAL "Metal backend built")

if(CYBERFLUIDS_METAL AND CYBERFLUIDS_HAVE_NUMPP)
    if(NOT APPLE)
        message(STATUS "Cyberfluids: CYBERFLUIDS_METAL is ON but this is not Apple — skipping Metal")
        return()
    endif()

    enable_language(OBJCXX)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)

    add_library(cyberfluids_metal STATIC src/metal/lid_driven_cavity_metal.mm)
    set_target_properties(cyberfluids_metal PROPERTIES
        OBJCXX_STANDARD 20 OBJCXX_STANDARD_REQUIRED ON)
    set_source_files_properties(src/metal/lid_driven_cavity_metal.mm PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc")
    target_include_directories(cyberfluids_metal PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(cyberfluids_metal PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK})
    target_compile_definitions(cyberfluids_metal PUBLIC CYBERFLUIDS_WITH_METAL)

    set(CYBERFLUIDS_HAVE_METAL ON CACHE INTERNAL "Metal backend built")
    message(STATUS "Cyberfluids: Metal backend enabled (Objective-C++ / system Metal framework)")
endif()
