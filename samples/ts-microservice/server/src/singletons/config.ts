import { EConfigTemplate, validators, ICoreConfig } from "ucconfig";
import dotenv from "dotenv";
import { Common } from "../lib/common";
import { EASN1TransportEncoding } from "../stub/TSROSEBase";

/**
 * The static config of our server
 */
class StaticConfig {
	public readonly clientConnection = {
		client_keepalive: 30000
	};

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
}

export type IConfig = IEconfConfig & ICoreConfig

/**
 *
 */
export class Config extends EConfigTemplate {
	private _config: IEconfConfig;

	/**
	 * Constructor --- you know?
	 */
	public constructor() {
		super("MICROSERVICE");
		dotenv.config();
		this.initCore();
		this._config = this.init();
		this.validate(true);
	}

	/**
	 * Getter for the main config
	 * @returns - IConfig object
	 */
	public get config(): IConfig {
		return { ...this._config, ...this.coreConfig };
	}

	/**
	 * Inits all configurations/settings from given environment variables
	 * @returns - Instance of this config
	 */
	private init(): IEconfConfig {
		const config: IEconfConfig = {
			...new StaticConfig(),
			instanceID: Common.generateGUID(),
			logDirectory: this.newProperty<string>("LOG_DIRECTORY", validators.validateFolderExists()),
			econfServerFDQN: this.newEnvProperty<string>("SERVER_FQDN", validators.validateStringNotEmpty("tolower"), undefined, true),
			econfServerListenPortTCP: this.newProperty<number>("SERVER_LISTEN_PORT_TCP", validators.validatePort(), 0),
			econfServerListenPortTLS: this.newProperty<number>("SERVER_LISTEN_PORT_TLS", validators.validatePort(), 0),
			econfServerListenIP: this.newProperty<string>("SERVER_LISTEN_IP", validators.validateIPv4(), "0.0.0.0"),
			econfServerCertfile: this.newProperty<string | undefined>("SERVER_CERTFILE", validators.validateFileExists(), undefined),
			econfServerKeyfile: this.newProperty<string | undefined>("SERVER_KEYFILE", validators.validateFileExists(), undefined),
			econfDefaultEncoding: this.newProperty<EASN1TransportEncoding>("SERVER_DEFAULTENCODING", validators.validateInteger(EASN1TransportEncoding.JSON, EASN1TransportEncoding.BER), EASN1TransportEncoding.BER)
		};

		return config;
	}

	/**
	 * Current working directory of the Node.js process
	 * @returns - current working directory of the Node.js process
	 */
	public get rootdir(): string {
		return process.cwd();
	}
}
