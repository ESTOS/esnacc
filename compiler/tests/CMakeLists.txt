include(FetchContent)

find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_Declare(
		googletest
		URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
	)
	FetchContent_MakeAvailable(googletest)
endif()

set(COMPILER_TEST_FIXTURES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
file(TO_CMAKE_PATH "${COMPILER_TEST_FIXTURES_DIR}" FIXTURES_DIR)
configure_file(test_config.h.in "${CMAKE_CURRENT_BINARY_DIR}/test_config.h" @ONLY)

add_executable(compiler-cli-tests
	cli_parameter_tests.cpp
	test_support.cpp
	test_work_dir.cpp
)
target_include_directories(compiler-cli-tests PRIVATE
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${CMAKE_SOURCE_DIR}"
	"${CMAKE_SOURCE_DIR}/compiler/core"
)
target_link_libraries(compiler-cli-tests PRIVATE GTest::gtest_main)
add_dependencies(compiler-cli-tests compiler)

include(GoogleTest)
gtest_discover_tests(compiler-cli-tests
	PROPERTIES
		ENVIRONMENT "ESNACC_EXECUTABLE=$<TARGET_FILE:compiler>"
)

add_executable(interface-baseline-tests
	interface_baseline_tests.cpp
	"${CMAKE_CURRENT_SOURCE_DIR}/../core/interface_baseline.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/../core/time_helpers.c"
)
target_include_directories(interface-baseline-tests PRIVATE
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${CMAKE_SOURCE_DIR}"
	"${CMAKE_SOURCE_DIR}/compiler/core"
	"${CMAKE_SOURCE_DIR}/c-lib/include"
)
target_compile_definitions(interface-baseline-tests PRIVATE COMPILER)
target_link_libraries(interface-baseline-tests PRIVATE GTest::gtest_main)
gtest_discover_tests(interface-baseline-tests)
