import dotenv from "dotenv";
import { cleanEnv, makeValidator, str } from "envalid";
import fs from "node:fs";

import { Common } from "../lib/common.js";
import { EASN1TransportEncoding } from "../stub/TSInvokeContext.js";

/**
 * Values that do not come from the environment.
 * Kept beside the env fields so the rest of the sample can read one IConfig.
 */
class StaticConfig {
	public readonly clientConnection = { client_keepalive: 30000 };

	public readonly bPrettyPrintMessages = true;
	public readonly logStatisticsOnConsole = false;
	public readonly debugObjectCreationDestruction = true;
}

interface IEconfConfig extends StaticConfig {
	// The dynamic instanceid, set when we start
	instanceID: string;
	// The logger directory
	logDirectory: string;
	// the name under which the server is public resolveable, reachable
	econfServerFDQN: string;
	// the internal listen Port for TCP (http://, ws://)
	econfServerListenPortTCP: number;
	// the internal listen Port for TLS (https://, wss://)
	econfServerListenPortTLS: number;
	// The listen ip of the server (127.0.0.1, leave empty to listen on all ip addresses)
	econfServerListenIP: string;
	// The cerficiate file in case we are offering TLS
	econfServerCertfile: string | undefined;
	// The key file in case we are offering TLS
	econfServerKeyfile: string | undefined;
	// The encoding the server is using as default
	econfDefaultEncoding: EASN1TransportEncoding;
	// development, staging, or production
	environment: "development" | "staging" | "production";
}

export type IConfig = IEconfConfig;

/**
 * A TCP or TLS port. 0 means that listener is off.
 */
const portValue = makeValidator((input: string) => {
	const value = Number(input);
	if (!Number.isInteger(value) || value < 0 || value > 65535)
		throw new Error("expected a port from 0 to 65535");
	return value;
});

/**
 * Replaces ${NAME} with the current environment value.
 * The sample .env uses that form for the host name.
 */
function expandEnvRefs(value: string): string {
	return value.replace(/\$\{([^}]+)\}/g, (_match, name: string) => process.env[name] ?? "");
}

/**
 * Reads MICROSERVICE_* from the environment and checks the paths.
 * Called once from the Config constructor after dotenv has loaded .env.
 * A missing required variable or a bad port throws before the server listens.
 */
function readConfig(): IConfig {
	for (const [key, value] of Object.entries(process.env)) {
		if (key.startsWith("MICROSERVICE_") && typeof value === "string" && value.includes("${"))
			process.env[key] = expandEnvRefs(value);
	}

	const env = cleanEnv(process.env, {
		MICROSERVICE_ENVIRONMENT: str({ choices: ["development", "staging", "production"] }),
		MICROSERVICE_VERSION_BUILD_DATE: str(),
		MICROSERVICE_VERSION_TAG: str(),
		MICROSERVICE_LOG_DIRECTORY: str(),
		MICROSERVICE_SERVER_FQDN: str(),
		MICROSERVICE_SERVER_LISTEN_PORT_TCP: portValue({ default: 0 }),
		MICROSERVICE_SERVER_LISTEN_PORT_TLS: portValue({ default: 0 }),
		MICROSERVICE_SERVER_LISTEN_IP: str({ default: "0.0.0.0" }),
		MICROSERVICE_SERVER_CERTFILE: str({ default: "" }),
		MICROSERVICE_SERVER_KEYFILE: str({ default: "" }),
		MICROSERVICE_SERVER_DEFAULTENCODING: portValue({ default: EASN1TransportEncoding.BER }),
	});

	const logDirectory = env.MICROSERVICE_LOG_DIRECTORY;
	if (!fs.existsSync(logDirectory) || !fs.statSync(logDirectory).isDirectory())
		throw new Error(`MICROSERVICE_LOG_DIRECTORY is not a directory: ${logDirectory}`);

	const fqdn = env.MICROSERVICE_SERVER_FQDN.trim().toLowerCase();
	if (!fqdn)
		throw new Error("MICROSERVICE_SERVER_FQDN is empty");

	const listenIP = env.MICROSERVICE_SERVER_LISTEN_IP;
	if (!/^(?:(?:25[0-5]|2[0-4]\d|1?\d?\d)\.){3}(?:25[0-5]|2[0-4]\d|1?\d?\d)$/.test(listenIP))
		throw new Error(`MICROSERVICE_SERVER_LISTEN_IP is not an IPv4 address: ${listenIP}`);

	const certfile = env.MICROSERVICE_SERVER_CERTFILE || undefined;
	const keyfile = env.MICROSERVICE_SERVER_KEYFILE || undefined;
	if (certfile && !fs.existsSync(certfile))
		throw new Error(`MICROSERVICE_SERVER_CERTFILE does not exist: ${certfile}`);
	if (keyfile && !fs.existsSync(keyfile))
		throw new Error(`MICROSERVICE_SERVER_KEYFILE does not exist: ${keyfile}`);

	const encoding = env.MICROSERVICE_SERVER_DEFAULTENCODING;
	if (encoding !== EASN1TransportEncoding.JSON && encoding !== EASN1TransportEncoding.BER)
		throw new Error("MICROSERVICE_SERVER_DEFAULTENCODING must be 1 (JSON) or 2 (BER)");

	return {
		...new StaticConfig(),
		instanceID: Common.generateGUID(),
		logDirectory,
		econfServerFDQN: fqdn,
		econfServerListenPortTCP: env.MICROSERVICE_SERVER_LISTEN_PORT_TCP,
		econfServerListenPortTLS: env.MICROSERVICE_SERVER_LISTEN_PORT_TLS,
		econfServerListenIP: listenIP,
		econfServerCertfile: certfile,
		econfServerKeyfile: keyfile,
		econfDefaultEncoding: encoding,
		environment: env.MICROSERVICE_ENVIRONMENT,
	};
}

/**
 * Loads .env and exposes the sample configuration.
 * globals.ts constructs one instance. The rest of the server reads config.
 */
export class Config {
	private readonly parsed: IConfig;

	/**
	 * Loads dotenv, then reads and checks MICROSERVICE_*.
	 * Throws when a required variable is missing or a path or port is invalid.
	 */
	public constructor() {
		dotenv.config();
		this.parsed = readConfig();
	}

	/**
	 * The configuration the rest of the sample reads.
	 */
	public get config(): IConfig {
		return this.parsed;
	}

	/**
	 * Current working directory of the Node.js process.
	 */
	public get rootdir(): string {
		return process.cwd();
	}
}
