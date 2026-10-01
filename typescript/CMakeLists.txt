# TypeScript glue tests (node:test specs + optional per-case CI CTest entries).
#
# Spec files: typescript/*.test.ts — run locally via VS Code node:test Test Explorer
# (nodejs-testing.* in .vscode/settings.json) or `pnpm test` in typescript/.
#
# When SNACC_CTEST_REGISTER_TS=ON (CI), each top-level test() literal becomes its own
# ctest (scripts/list-ts-glue-tests.mjs + scripts/run-ts-glue-one-test.mjs). Local
# `pnpm test` still runs the whole suite in one process.

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

if(SNACC_CTEST_REGISTER_TS)
	# Fixture setup (not a behavioral test): regenerate sample stubs from ASN.1, install
	# pnpm deps (@estos/asn1ts, tsx, …), then copy gluecode into typescript/stub.
	# Individual glue ctests require this fixture and skip their own stub copy.
	add_test(
		NAME typescript_prepare_glue_test_environment_generate_stubs_and_install_deps
		COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target snacc-ts-glue-prepare
	)
	set_tests_properties(
		typescript_prepare_glue_test_environment_generate_stubs_and_install_deps
		PROPERTIES
			FIXTURES_SETUP ts-glue-env
			LABELS "ts;glue"
	)

	# Discover the same top-level test() literals the VS Code Test Explorer shows.
	# Configure fails if a spec uses a dynamic name, so CI cannot silently drop a case.
	set(SNACC_TS_GLUE_LIST_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/list-ts-glue-tests.mjs")
	set(SNACC_TS_GLUE_ONE_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/run-ts-glue-one-test.mjs")
	execute_process(
		COMMAND "${SNACC_TS_NODE_EXECUTABLE}" "${SNACC_TS_GLUE_LIST_SCRIPT}"
		OUTPUT_VARIABLE SNACC_TS_GLUE_TEST_LISTING
		ERROR_VARIABLE SNACC_TS_GLUE_LIST_ERROR
		RESULT_VARIABLE SNACC_TS_GLUE_LIST_RESULT
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(NOT SNACC_TS_GLUE_LIST_RESULT EQUAL 0)
		message(FATAL_ERROR "Failed to list TypeScript glue tests: ${SNACC_TS_GLUE_LIST_ERROR}")
	endif()
	string(REPLACE "\r" "" SNACC_TS_GLUE_TEST_LISTING "${SNACC_TS_GLUE_TEST_LISTING}")
	string(REPLACE "\n" ";" SNACC_TS_GLUE_TEST_LINES "${SNACC_TS_GLUE_TEST_LISTING}")
	set(SNACC_TS_GLUE_TEST_COUNT 0)
	foreach(SNACC_TS_GLUE_TEST_LINE IN LISTS SNACC_TS_GLUE_TEST_LINES)
		if(SNACC_TS_GLUE_TEST_LINE STREQUAL "")
			continue()
		endif()
		string(FIND "${SNACC_TS_GLUE_TEST_LINE}" "\t" SNACC_TS_GLUE_TAB)
		if(SNACC_TS_GLUE_TAB LESS 0)
			message(FATAL_ERROR "Bad TypeScript glue test listing line: ${SNACC_TS_GLUE_TEST_LINE}")
		endif()
		string(SUBSTRING "${SNACC_TS_GLUE_TEST_LINE}" 0 "${SNACC_TS_GLUE_TAB}" SNACC_TS_GLUE_FILE)
		math(EXPR SNACC_TS_GLUE_NAME_AT "${SNACC_TS_GLUE_TAB} + 1")
		string(SUBSTRING "${SNACC_TS_GLUE_TEST_LINE}" "${SNACC_TS_GLUE_NAME_AT}" -1 SNACC_TS_GLUE_NAME)
		string(REGEX REPLACE "\\.test\\.ts$" "" SNACC_TS_GLUE_STEM "${SNACC_TS_GLUE_FILE}")
		set(SNACC_TS_GLUE_CTEST_NAME "typescript/${SNACC_TS_GLUE_STEM}/${SNACC_TS_GLUE_NAME}")
		add_test(
			NAME "${SNACC_TS_GLUE_CTEST_NAME}"
			COMMAND "${SNACC_TS_NODE_EXECUTABLE}" "${SNACC_TS_GLUE_ONE_SCRIPT}" "${SNACC_TS_GLUE_FILE}" "${SNACC_TS_GLUE_NAME}"
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		)
		set_tests_properties(
			"${SNACC_TS_GLUE_CTEST_NAME}"
			PROPERTIES
				LABELS "ts;glue"
				ENVIRONMENT "PATH=$ENV{PATH};SNACC_TS_GLUE_STUB_PREPARED=1"
				FIXTURES_REQUIRED ts-glue-env
		)
		math(EXPR SNACC_TS_GLUE_TEST_COUNT "${SNACC_TS_GLUE_TEST_COUNT} + 1")
	endforeach()
	if(SNACC_TS_GLUE_TEST_COUNT EQUAL 0)
		message(FATAL_ERROR "No TypeScript glue tests discovered")
	endif()
	message(STATUS "Registered ${SNACC_TS_GLUE_TEST_COUNT} TypeScript glue CTest entries")
endif()
