# Standalone build for the Palabos oracle generator. Builds only the Palabos
# static library + this one program (not the whole Palabos example suite).
# Serial (no MPI), no HDF5. Requires TBB.
#
#   cmake -S . -B build -DPALABOS_ROOT=/path/to/palabos
#   cmake --build build -j
#   ./build/cavity2d_oracle        # writes cavity2d_palabos.csv
cmake_minimum_required(VERSION 3.16)
project(cyberfluids_palabos_oracle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(PALABOS_ROOT "" CACHE PATH "Path to the Palabos source tree")
if(NOT EXISTS "${PALABOS_ROOT}/src")
    message(FATAL_ERROR "Set -DPALABOS_ROOT to a Palabos checkout (got '${PALABOS_ROOT}')")
endif()

add_definitions(-DPLB_USE_POSIX)
if(APPLE)
    add_definitions(-DPLB_MAC_OS_X)
endif()

# Palabos uses std::execution::par_unseq; libc++ gates the parallel algorithms
# behind -fexperimental-library. Apply it to every translation unit.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fexperimental-library" HAS_EXPERIMENTAL_LIBRARY)
if(HAS_EXPERIMENTAL_LIBRARY)
    add_compile_options(-fexperimental-library)
    add_link_options(-fexperimental-library)
endif()

include_directories(
    ${PALABOS_ROOT}/src
    ${PALABOS_ROOT}/externalLibraries
    ${PALABOS_ROOT}/externalLibraries/Eigen3)

file(GLOB_RECURSE PALABOS_SRC ${PALABOS_ROOT}/src/*.cpp)
file(GLOB_RECURSE TINYXML_SRC ${PALABOS_ROOT}/externalLibraries/tinyxml/*.cpp)
add_library(palabos STATIC ${PALABOS_SRC} ${TINYXML_SRC})

find_package(TBB REQUIRED)

add_executable(cavity2d_oracle cavity2d_oracle.cpp)
target_link_libraries(cavity2d_oracle palabos TBB::tbb)

add_executable(cavity3d_oracle cavity3d_oracle.cpp)
target_link_libraries(cavity3d_oracle palabos TBB::tbb)
