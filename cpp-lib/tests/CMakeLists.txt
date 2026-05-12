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
	DEPENDS compiler ${SNACC_SAMPLE_ASN1}
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT "Generating C++ sample ROSE stubs for tests"
	VERBATIM
)

add_library(snacc-sample-generated STATIC ${SNACC_SAMPLE_GENERATED_FILES})
target_include_directories(snacc-sample-generated PUBLIC "${SNACC_SAMPLE_TEST_GENERATED_DIR}" "${CMAKE_SOURCE_DIR}")
target_link_libraries(snacc-sample-generated PUBLIC snacc_cpp_lib)

add_executable(cpp-lib-sample-runtime-tests
	call_flow_tests.cpp
	logging_tests.cpp
	logical_failure_tests.cpp
	public_api_tests.cpp
	telemetry_tests.cpp
	transport_failure_tests.cpp
	lifecycle_tests.cpp
)
target_include_directories(cpp-lib-sample-runtime-tests PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(cpp-lib-sample-runtime-tests PRIVATE snacc-sample-generated GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cpp-lib-sample-runtime-tests)
