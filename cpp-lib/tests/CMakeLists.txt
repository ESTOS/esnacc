include("${CMAKE_SOURCE_DIR}/cmake/snacc_gtest.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/snacc_compiler_cpp_test_sources.cmake")

set(SNACC_SAMPLE_TEST_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(SNACC_SAMPLE_ASN1
	"${CMAKE_SOURCE_DIR}/samples/interface/ENetUC_Common.asn1"
	"${CMAKE_SOURCE_DIR}/samples/interface/ENetUC_Event_Manager.asn1"
	"${CMAKE_SOURCE_DIR}/samples/interface/ENetUC_Settings_Manager.asn1"
)

set(SNACC_SAMPLE_GENERATED_FILES
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Common.cpp"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Common.h"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_CommonDecl.h"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Event_Manager.cpp"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Event_Manager.h"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Event_ManagerDecl.h"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Event_ManagerROSE.cpp"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Event_ManagerROSE.h"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Event_ManagerROSEInterface.h"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Settings_Manager.cpp"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Settings_Manager.h"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Settings_ManagerDecl.h"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Settings_ManagerROSE.cpp"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Settings_ManagerROSE.h"
	"${SNACC_SAMPLE_TEST_GENERATED_DIR}/ENetUC_Settings_ManagerROSEInterface.h"
)

add_custom_command(
	OUTPUT ${SNACC_SAMPLE_GENERATED_FILES}
	COMMAND ${CMAKE_COMMAND} -E make_directory "${SNACC_SAMPLE_TEST_GENERATED_DIR}"
	COMMAND $<TARGET_FILE:compiler> -C -R -j -o "${SNACC_SAMPLE_TEST_GENERATED_DIR}" ${SNACC_SAMPLE_ASN1}
	DEPENDS compiler $<TARGET_FILE:compiler> ${SNACC_SAMPLE_ASN1}
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT "Generating C++ sample ROSE stubs for tests"
	VERBATIM
)

add_library(snacc-sample-generated STATIC ${SNACC_SAMPLE_GENERATED_FILES})
target_include_directories(snacc-sample-generated PUBLIC "${SNACC_SAMPLE_TEST_GENERATED_DIR}" "${CMAKE_SOURCE_DIR}")
target_link_libraries(snacc-sample-generated PUBLIC snacc_cpp_lib)

set(SNACC_RUNTIME_CPP_TEST_SOURCES
	async_invoke/async_invoke_tests.cpp
	call_flow/call_flow_tests.cpp
	invoke_block_policy/client_invoke_block_policy_tests.cpp
	invoke_block_policy/server_invoke_block_policy_tests.cpp
	invoke_context/invoke_context_tests.cpp
	lifecycle/lifecycle_tests.cpp
	logging/logging_tests.cpp
	logical_failure/logical_failure_tests.cpp
	module_registry/module_registry_tests.cpp
	public_api/public_api_tests.cpp
	rose_session_subscription/rose_session_subscription_tests.cpp
	telemetry/telemetry_tests.cpp
	transport_failure/transport_failure_tests.cpp
)

# Single GTest executable: compiler unit tests + cpp-lib ROSE runtime tests.
add_executable(snacc-cpp-tests
	${SNACC_RUNTIME_CPP_TEST_SOURCES}
	${SNACC_COMPILER_CPP_TEST_SOURCES}
)
target_include_directories(snacc-cpp-tests PRIVATE
	"${CMAKE_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}"
	${SNACC_COMPILER_CPP_TEST_INCLUDE_DIRS}
)
target_compile_definitions(snacc-cpp-tests PRIVATE
	SNACC_GTEST_BINARY
	${SNACC_COMPILER_CPP_TEST_COMPILE_DEFINITIONS}
)
target_link_libraries(snacc-cpp-tests PRIVATE snacc-sample-generated GTest::gtest_main c-lib)
add_dependencies(snacc-cpp-tests compiler)

include(GoogleTest)
gtest_discover_tests(snacc-cpp-tests
	PROPERTIES
		ENVIRONMENT "ESNACC_EXECUTABLE=$<TARGET_FILE:compiler>"
)

# Backward-compatible alias for scripts and docs that still reference the old target name.
add_custom_target(cpp-lib-sample-runtime-tests)
add_dependencies(cpp-lib-sample-runtime-tests snacc-cpp-tests)
