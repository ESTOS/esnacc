# TypeScript glue tests (node:test specs + optional CI CTest batch wrapper).
#
# Spec files: typescript/*.test.ts — run locally via VS Code node:test Test Explorer
# (nodejs-testing.* in .vscode/settings.json) or `pnpm test` in typescript/.
#
# When SNACC_CTEST_REGISTER_TS=ON (CI), the batch ctest below runs the same files via
# scripts/run-ts-glue-test-suite.mjs. Do not duplicate specs here; CMake is only a CI gate.

if(NOT BUILD_TESTING)
	return()
endif()

set(SNACC_TS_GLUE_SUITE_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/run-ts-glue-test-suite.mjs")
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
	COMMAND ${CMAKE_COMMAND} -E touch "${SNACC_TS_GLUE_PREPARE_STAMP}"
	DEPENDS compiler
	WORKING_DIRECTORY "${SNACC_SAMPLES_ROOT}"
	COMMENT "Preparing TypeScript glue test dependencies"
	VERBATIM
)

add_custom_target(snacc-ts-glue-prepare DEPENDS "${SNACC_TS_GLUE_PREPARE_STAMP}")

if(SNACC_CTEST_REGISTER_TS)
	# Fixture setup (not a behavioral test): regenerate typescript/stub from ASN.1 via
	# samples/prepare.js and install pnpm deps (@estos/asn1ts, tsx, …). Required before the
	# glue batch test; skipped automatically when the fixture already ran in the same ctest session.
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

	# CI batch: same typescript/*.test.ts files as node:test Test Explorer (run-ts-glue-test-suite.mjs).
	add_test(
		NAME typescript_run_all_compiler_ts_rose_runtime_loopback_spec_tests
		COMMAND "${SNACC_TS_NODE_EXECUTABLE}" "${SNACC_TS_GLUE_SUITE_SCRIPT}"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)

	set_tests_properties(
		typescript_run_all_compiler_ts_rose_runtime_loopback_spec_tests
		PROPERTIES
			LABELS "ts;glue"
			ENVIRONMENT "PATH=$ENV{PATH}"
			FIXTURES_REQUIRED ts-glue-env
	)
endif()
