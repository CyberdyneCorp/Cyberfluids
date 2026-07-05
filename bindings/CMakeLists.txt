# Binding smoke tests (task 9.4). Registered only when the toolchain is present.
# Both wrap the cyberfluids_c shared library; build it before running CTest.

# --- Python (ctypes + NumPy) ---
find_program(CYBERFLUIDS_PYTHON3 python3)
if(CYBERFLUIDS_PYTHON3)
    execute_process(COMMAND ${CYBERFLUIDS_PYTHON3} -c "import numpy"
                    RESULT_VARIABLE _cf_numpy_missing OUTPUT_QUIET ERROR_QUIET)
    if(_cf_numpy_missing EQUAL 0)
        add_test(NAME python_binding
            COMMAND ${CMAKE_COMMAND} -E env
                "CYBERFLUIDS_LIBRARY=$<TARGET_FILE:cyberfluids_c>"
                "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/python"
                ${CYBERFLUIDS_PYTHON3} ${CMAKE_CURRENT_SOURCE_DIR}/python/smoke_test.py)
        # DLPack zero-copy interop (self-skips if numpy<1.23 / no from_dlpack).
        add_test(NAME python_dlpack
            COMMAND ${CMAKE_COMMAND} -E env
                "CYBERFLUIDS_LIBRARY=$<TARGET_FILE:cyberfluids_c>"
                "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/python"
                ${CYBERFLUIDS_PYTHON3} ${CMAKE_CURRENT_SOURCE_DIR}/python/test_dlpack.py)
        # Wind-tunnel binding smoke (analytic sphere; no geometry build needed).
        add_test(NAME python_wind_tunnel
            COMMAND ${CMAKE_COMMAND} -E env
                "CYBERFLUIDS_LIBRARY=$<TARGET_FILE:cyberfluids_c>"
                "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/python"
                ${CYBERFLUIDS_PYTHON3} ${CMAKE_CURRENT_SOURCE_DIR}/python/test_wind_tunnel.py)
    else()
        message(STATUS "Cyberfluids: numpy not available; skipping Python binding test")
    endif()
endif()

# --- Swift (SwiftPM system-library module) ---
find_program(CYBERFLUIDS_SWIFT swift)
if(CYBERFLUIDS_SWIFT)
    add_test(NAME swift_binding
        COMMAND ${CYBERFLUIDS_SWIFT} run --package-path ${CMAKE_CURRENT_SOURCE_DIR}/swift
            -Xcc -I${CMAKE_SOURCE_DIR}/include
            -Xlinker -L$<TARGET_FILE_DIR:cyberfluids_c>
            -Xlinker -rpath -Xlinker $<TARGET_FILE_DIR:cyberfluids_c>
            -Xlinker -rpath -Xlinker ${CYBERFLUIDS_DEPS_PREFIX}/lib
            cavity-demo)
endif()
