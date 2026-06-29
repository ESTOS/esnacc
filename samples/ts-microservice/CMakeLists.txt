if(NOT BUILD_TESTING)
	return()
endif()

set(SNACC_TS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(SNACC_TS_NODE_SERVER "${SNACC_TS_ROOT}/node-server")
set(SNACC_TS_NODE_CLIENT "${SNACC_TS_ROOT}/node-client")
set(SNACC_TS_PREPARE_STAMP "${CMAKE_CURRENT_BINARY_DIR}/ts-prepare.stamp")
set(SNACC_TS_SERVER_BUILD_STAMP "${CMAKE_CURRENT_BINARY_DIR}/ts-server-build.stamp")
set(SNACC_TS_CLIENT_BUILD_STAMP "${CMAKE_CURRENT_BINARY_DIR}/ts-client-build.stamp")
set(SNACC_TS_LOG_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ts-test-logs")

find_program(SNACC_TS_PNPM_EXECUTABLE pnpm)
find_program(SNACC_TS_NODE_EXECUTABLE node)

if(NOT SNACC_TS_PNPM_EXECUTABLE)
	message(WARNING "pnpm not found; TypeScript integration tests will be skipped")
	return()
endif()

if(NOT SNACC_TS_NODE_EXECUTABLE)
	message(WARNING "node not found; TypeScript integration tests will be skipped")
	return()
endif()

set(SNACC_SAMPLES_ROOT "${CMAKE_SOURCE_DIR}/samples")

add_custom_command(
	OUTPUT "${SNACC_TS_PREPARE_STAMP}"
	COMMAND ${CMAKE_COMMAND} -E env
		"SNACC_COMPILER=$<TARGET_FILE:compiler>"
		"CMAKE_COMPILER_TARGET=$<TARGET_FILE:compiler>"
		"SNACC_PREPARE_FROZEN=1"
		"${SNACC_TS_NODE_EXECUTABLE}" "${SNACC_SAMPLES_ROOT}/prepare.js" "--frozen-lockfile"
	COMMAND ${CMAKE_COMMAND} -E touch "${SNACC_TS_PREPARE_STAMP}"
	DEPENDS compiler
	WORKING_DIRECTORY "${SNACC_SAMPLES_ROOT}"
	COMMENT "Preparing TypeScript samples (stubs and pnpm install)"
	VERBATIM
)

add_custom_target(snacc-ts-prepare DEPENDS "${SNACC_TS_PREPARE_STAMP}")

add_custom_command(
	OUTPUT "${SNACC_TS_SERVER_BUILD_STAMP}"
	COMMAND "${SNACC_TS_PNPM_EXECUTABLE}" run build
	COMMAND ${CMAKE_COMMAND} -E touch "${SNACC_TS_SERVER_BUILD_STAMP}"
	DEPENDS snacc-ts-prepare
	WORKING_DIRECTORY "${SNACC_TS_NODE_SERVER}"
	COMMENT "Building TypeScript node-server sample"
	VERBATIM
)

add_custom_target(snacc-ts-server-build DEPENDS "${SNACC_TS_SERVER_BUILD_STAMP}")

add_custom_command(
	OUTPUT "${SNACC_TS_CLIENT_BUILD_STAMP}"
	COMMAND "${SNACC_TS_PNPM_EXECUTABLE}" run build
	COMMAND ${CMAKE_COMMAND} -E touch "${SNACC_TS_CLIENT_BUILD_STAMP}"
	DEPENDS snacc-ts-prepare
	WORKING_DIRECTORY "${SNACC_TS_NODE_CLIENT}"
	COMMENT "Building TypeScript node-client integration tests"
	VERBATIM
)

add_custom_target(snacc-ts-client-build DEPENDS "${SNACC_TS_CLIENT_BUILD_STAMP}")

add_custom_target(snacc-ts-all DEPENDS snacc-ts-server-build snacc-ts-client-build)

add_test(
	NAME snacc-ts-integration
	COMMAND "${SNACC_TS_NODE_EXECUTABLE}" --test "${SNACC_TS_NODE_CLIENT}/dist/run-tests.js"
	WORKING_DIRECTORY "${SNACC_TS_NODE_CLIENT}"
)

set_tests_properties(
	snacc-ts-integration
	PROPERTIES
		LABELS "ts;integration"
		ENVIRONMENT "SNACC_TS_SERVER_ROOT=${SNACC_TS_NODE_SERVER};SNACC_TS_TEST_PORT=13020;SNACC_TS_LOG_DIRECTORY=${SNACC_TS_LOG_DIRECTORY}"
)
