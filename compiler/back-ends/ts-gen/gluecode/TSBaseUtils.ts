// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do NOT edit or modify this code as it is machine generated
// and will be overwritten with every code generation of the esnacc.exe

// dprint-ignore-file
/* eslint-disable */

/**
 * True when Node was started under the built-in test runner (CLI, workers, or Test Explorer).
 * Returns false in browser bundles where `process` is absent (breakpoints behave as before).
 */
export function isNodeTestEnvironment(): boolean {
	const proc = typeof process === "undefined" ? undefined : process;
	if (!proc?.env)
		return false;

	if (proc.env["SNACC_ROSE_DEBUG_IN_TESTS"] === "1" || proc.env["SNACC_ROSE_DEBUG_IN_TESTS"] === "true")
		return false;

	const testContext = proc.env["NODE_TEST_CONTEXT"];
	if (testContext !== undefined && testContext !== "")
		return true;

	const flags = [...(proc.argv ?? []), ...(proc.execArgv ?? [])];
	return flags.some((arg) => arg === "--test" || arg.startsWith("--test-"));
}

/** Break into an attached debugger unless this process is running node:test. */
export function roseDebugBreak(): void {
	if (isNodeTestEnvironment())
		return;
	else
		debugger;
}
