# TypeScript glue tests.
#
# Spec files: typescript/*.test.ts. VS Code, `pnpm test`, and CI all run
# `node --enable-source-maps --test` on the emitted typescript/dist JavaScript
# (scripts/run-ts-glue-test-suite.mjs). This directory only prepares stubs when
# the compiler or ASN.1 inputs change. It does not register per-case CTest entries.

if(NOT BUILD_TESTING)
	return()
endif()

find_program(SNACC_TS_NODE_EXECUTABLE node)

if(NOT SNACC_TS_NODE_EXECUTABLE)
	message(WARNING "node not found; TypeScript glue tests will be skipped")
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
	COMMAND "${SNACC_TS_NODE_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare-ts-glue-stub.mjs"
	COMMAND ${CMAKE_COMMAND} -E touch "${SNACC_TS_GLUE_PREPARE_STAMP}"
	DEPENDS compiler "${CMAKE_SOURCE_DIR}/scripts/prepare-ts-glue-stub.mjs"
	WORKING_DIRECTORY "${SNACC_SAMPLES_ROOT}"
	COMMENT "Preparing TypeScript glue test dependencies"
	VERBATIM
)

add_custom_target(snacc-ts-glue-prepare DEPENDS "${SNACC_TS_GLUE_PREPARE_STAMP}")
