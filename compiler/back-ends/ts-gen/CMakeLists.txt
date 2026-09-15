if(NOT BUILD_TESTING)
	return()
endif()

set(SNACC_TS_GLUE_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/run_gluecode_tests.sh")
find_program(SNACC_TS_NODE_EXECUTABLE node)
find_program(SNACC_TS_BASH_EXECUTABLE bash)

if(NOT SNACC_TS_NODE_EXECUTABLE)
	message(WARNING "node not found; TypeScript glue tests will be skipped")
	return()
endif()

if(NOT SNACC_TS_BASH_EXECUTABLE)
	message(WARNING "bash not found; TypeScript glue tests will be skipped on this platform")
	return()
endif()

set(SNACC_TS_GLUE_PREPARE_STAMP "${CMAKE_CURRENT_BINARY_DIR}/ts-glue-prepare.stamp")
set(SNACC_SAMPLES_ROOT "${CMAKE_SOURCE_DIR}/samples")

add_custom_command(
	OUTPUT "${SNACC_TS_GLUE_PREPARE_STAMP}"
	COMMAND ${CMAKE_COMMAND} -E env
		"SNACC_COMPILER=$<TARGET_FILE:compiler>"
		"CMAKE_COMPILER_TARGET=$<TARGET_FILE:compiler>"
		"SNACC_PREPARE_FROZEN=1"
		"${SNACC_TS_NODE_EXECUTABLE}" "${SNACC_SAMPLES_ROOT}/prepare.js" "--frozen-lockfile"
	COMMAND ${CMAKE_COMMAND} -E touch "${SNACC_TS_GLUE_PREPARE_STAMP}"
	DEPENDS compiler
	WORKING_DIRECTORY "${SNACC_SAMPLES_ROOT}"
	COMMENT "Preparing TypeScript glue test dependencies"
	VERBATIM
)

add_custom_target(snacc-ts-glue-prepare DEPENDS "${SNACC_TS_GLUE_PREPARE_STAMP}")

add_test(
	NAME snacc-ts-glue
	COMMAND "${SNACC_TS_BASH_EXECUTABLE}" "${SNACC_TS_GLUE_SCRIPT}"
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_tests_properties(
	snacc-ts-glue
	PROPERTIES
		LABELS "ts;glue"
		ENVIRONMENT "PATH=$ENV{PATH}"
)

add_dependencies(snacc-ts-glue snacc-ts-glue-prepare)
