# Runnable demos. Built only when NumPP is available (they use the NumPP-backed
# solver). Not registered as tests — behavioral coverage lives in tests/.

foreach(_e cavity2d cavity3d poiseuille2d)
    add_executable(${_e} ${_e}.cpp)
    target_link_libraries(${_e} PRIVATE cyberfluids_core numpp::numpp)
endforeach()
