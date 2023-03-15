import { TSASN1Server } from "../stub/TSASN1Server";
import { EASN1TransportEncoding } from "../stub/TSROSEBase";
import { ILogData } from "uclogger";
import { theConfig } from "../globals";
import { EventManager } from "../handlers/eventManager";
import { SettingsManager } from "../handlers/settingsManager";

/**
 * Receiver and sender singleton
 *
 * Implements the different asn1 interfaces as public members.
 * Other singletons can call methods and events to send data to a client through these members.
 * The receive method is handling data provided from a client and calling the appropriate method.
 */
export class Server extends TSASN1Server {
	// The singleton instance of this class
	private static instance: Server;
	// The different managers for the rose interfaces
	public readonly eventManager: EventManager;
	public readonly settingsManager: SettingsManager;

	/**
	 * Constructs Server.
	 * Method is private as we follow the Singleton Approach using getInstance
	 */
	private constructor() {
		super(EASN1TransportEncoding.BER);
		this.encodeContext.bPrettyPrint = theConfig.bPrettyPrintMessages;
		// These are the handling modules the server is offering to the clients
		// The modules hold the rose implementation capsuled as member but use this class as transport layer
		this.eventManager = new EventManager(this);
		this.settingsManager = new SettingsManager(this);
	}

	/**
	 * Gets instance of Server to use as singleton.
	 *
	 * @returns - an instance of this class.
	 */
	public static getInstance(): Server {
		if (!Server.instance)
			Server.instance = new Server();
		return Server.instance;
	}

	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 *
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public override getLogData(): ILogData {
		return { className: "Server" };
	}

	/**
	 * Init of the singleton
	 */
	public init(): void {
	}

	/**
	 * Exit of the singleton
	 */
	public exit(): void {
	}
}
