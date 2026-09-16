# Generates test_config.h for compiler C++ tests (linked into snacc-cpp-tests).

include("${CMAKE_SOURCE_DIR}/cmake/snacc_compiler_cpp_test_sources.cmake")

set(COMPILER_TEST_FIXTURES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
file(TO_CMAKE_PATH "${COMPILER_TEST_FIXTURES_DIR}" FIXTURES_DIR)
configure_file(test_config.h.in "${CMAKE_CURRENT_BINARY_DIR}/test_config.h" @ONLY)
