# Compiler C++ unit test sources linked into snacc-cpp-tests (see cpp-lib/tests/CMakeLists.txt).

set(SNACC_COMPILER_CPP_TEST_SOURCES
	"${CMAKE_SOURCE_DIR}/compiler/tests/cpp/cli_parameter_tests.cpp"
	"${CMAKE_SOURCE_DIR}/compiler/tests/cpp/deprecated_successor_tests.cpp"
	"${CMAKE_SOURCE_DIR}/compiler/tests/cpp/deprecated_successor_cli_tests.cpp"
	"${CMAKE_SOURCE_DIR}/compiler/tests/cpp/interface_baseline_tests.cpp"
	"${CMAKE_SOURCE_DIR}/compiler/tests/cpp/test_support.cpp"
	"${CMAKE_SOURCE_DIR}/compiler/tests/cpp/test_work_dir.cpp"
	"${CMAKE_SOURCE_DIR}/compiler/core/snacc-deprecated-successor.cpp"
	"${CMAKE_SOURCE_DIR}/compiler/core/interface_baseline.c"
	"${CMAKE_SOURCE_DIR}/compiler/core/time_helpers.c"
)

set(SNACC_COMPILER_CPP_TEST_INCLUDE_DIRS
	"${CMAKE_BINARY_DIR}/compiler/tests/cpp"
	"${CMAKE_SOURCE_DIR}"
	"${CMAKE_SOURCE_DIR}/compiler/core"
	"${CMAKE_SOURCE_DIR}/c-lib/include"
)

set(SNACC_COMPILER_CPP_TEST_COMPILE_DEFINITIONS COMPILER)
