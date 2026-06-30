import fs from "node:fs";
import path from "node:path";

/** Required by ucconfig `initCore()` — see node-server/.env.sample */
const MICROSERVICE_CORE_ENV_KEYS = [
	"MICROSERVICE_VERSION_BUILD_DATE",
	"MICROSERVICE_VERSION_TAG",
	"MICROSERVICE_ENVIRONMENT",
] as const;

/** Required by node-server `Config` — see node-server/.env.sample */
const MICROSERVICE_SERVER_ENV_KEYS = [
	"MICROSERVICE_LOG_DIRECTORY",
	"MICROSERVICE_SERVER_FQDN",
	"MICROSERVICE_SERVER_LISTEN_PORT_TCP",
] as const;

export const REQUIRED_MICROSERVICE_SERVER_ENV_KEYS = [
	...MICROSERVICE_CORE_ENV_KEYS,
	"MICROSERVICE_LOG_LEVEL",
	"MICROSERVICE_LOG_TO_CONSOLE",
	...MICROSERVICE_SERVER_ENV_KEYS,
] as const;

/** Runner configuration for the headless integration client (not ucconfig). */
export const REQUIRED_INTEGRATION_RUNNER_ENV_KEYS = [
	"SNACC_TS_TEST_PORT",
	"SNACC_TS_SERVER_ROOT",
	"SNACC_TS_LOG_DIRECTORY",
] as const;

export interface MicroserviceServerTestEnvOptions {
	logDirectory: string;
	testPort: number;
	serverFqdn?: string;
	versionBuildDate?: string;
	versionTag?: string;
}

export interface IntegrationRunnerEnv {
	nodeServerRoot: string;
	testPort: number;
	logDirectory: string;
}

function assertEnvKeys(
	env: NodeJS.ProcessEnv,
	keys: readonly string[],
	label: string,
): void {
	const missing = keys.filter((key) => {
		const value = env[key];
		return value === undefined || value === "";
	});
	if (missing.length > 0) {
		throw new Error(
			`${label} is missing required environment variable(s): ${missing.join(", ")}. `
				+ "See samples/ts-microservice/node-server/.env.sample",
		);
	}
}

export function createMicroserviceServerEnv(
	options: MicroserviceServerTestEnvOptions,
): NodeJS.ProcessEnv {
	const env: NodeJS.ProcessEnv = {
		...process.env,
		MICROSERVICE_VERSION_BUILD_DATE: options.versionBuildDate ?? "06/30/2026",
		MICROSERVICE_VERSION_TAG: options.versionTag ?? "1.0.0",
		MICROSERVICE_ENVIRONMENT: "development",
		MICROSERVICE_LOG_LEVEL: "error",
		MICROSERVICE_LOG_TO_CONSOLE: "0",
		MICROSERVICE_LOG_DIRECTORY: options.logDirectory,
		MICROSERVICE_SERVER_FQDN: options.serverFqdn ?? "localhost",
		MICROSERVICE_SERVER_LISTEN_PORT_TCP: String(options.testPort),
	};

	assertEnvKeys(env, REQUIRED_MICROSERVICE_SERVER_ENV_KEYS, "node-server test environment");
	return env;
}

export function resolveIntegrationRunnerEnv(nodeClientRoot: string): IntegrationRunnerEnv {
	const nodeServerRoot = process.env.SNACC_TS_SERVER_ROOT
		?? path.resolve(nodeClientRoot, "../node-server");
	const testPort = Number(process.env.SNACC_TS_TEST_PORT ?? "13020");
	const logDirectory = process.env.SNACC_TS_LOG_DIRECTORY
		?? path.join(nodeServerRoot, "log", "integration-tests");

	const env: NodeJS.ProcessEnv = {
		...process.env,
		SNACC_TS_SERVER_ROOT: nodeServerRoot,
		SNACC_TS_TEST_PORT: String(testPort),
		SNACC_TS_LOG_DIRECTORY: logDirectory,
	};

	assertEnvKeys(env, REQUIRED_INTEGRATION_RUNNER_ENV_KEYS, "integration test runner environment");

	if (!Number.isInteger(testPort) || testPort <= 0 || testPort > 65535) {
		throw new Error(`SNACC_TS_TEST_PORT must be a valid TCP port, got: ${env.SNACC_TS_TEST_PORT}`);
	}

	return { nodeServerRoot, testPort, logDirectory };
}

export function assertNodeServerTestLayout(nodeServerRoot: string): void {
	const requiredPaths = [
		path.join(nodeServerRoot, "index.js"),
		path.join(nodeServerRoot, "dist", "src", "app.js"),
		path.join(nodeServerRoot, ".env.sample"),
	];
	const missing = requiredPaths.filter((entry) => !fs.existsSync(entry));
	if (missing.length > 0) {
		throw new Error(
			`node-server test layout is incomplete (build stubs/server first). Missing:\n${missing.join("\n")}`,
		);
	}
}
