import { theLogger } from "../globals";
import { IASN1ClientConnectionHandler } from "../stub/TSASN1Server";
import { ClientConnection, ClientConnections } from "../lib/clientConnection";
import { ILogData } from "uclogger";
import { IClientConnectionConstructorArguments } from "../lib/IClientConnection";

/**
 * The notify interface someone that wants to subscribe to the clientConnectionManager via addNotify has to implement
 */
export interface IClientConnectionNotify {
	on_ClientConnected: (con: ClientConnection) => void;
	on_ClientDisconnected: (con: ClientConnection) => void;
}

/**
 * Set of connection notifies
 */
class IClientConnectionNotifies extends Set<IClientConnectionNotify> {
}

/**
 * The client connection manager holds the list of active websocket client connections.
 *
 * Implements a notify that informs about connecting disconnecting clients.
 */
export class ClientConnectionManager implements IASN1ClientConnectionHandler {
	// The singleton instance of this class
	private static instance: ClientConnectionManager;
	private clientConnections: ClientConnections;
	private connectionNotifies: IClientConnectionNotifies;

	/**
	 * Constructs ClientConnectionManager.
	 * Method is private as we follow the Singleton Approach using getInstance
	 */
	private constructor() {
		this.onEvent_ClientConnectionDestroyed = this.onEvent_ClientConnectionDestroyed.bind(this);
		this.clientConnections = new ClientConnections();
		this.connectionNotifies = new IClientConnectionNotifies();
	}

	/**
	 * Gets instance of ClientConnectionManager to use as singleton.
	 *
	 * @returns - an instance of this class.
	 */
	public static getInstance(): ClientConnectionManager {
		if (!ClientConnectionManager.instance)
			ClientConnectionManager.instance = new ClientConnectionManager();
		return ClientConnectionManager.instance;
	}

	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 *
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public getLogData(): ILogData {
		return {
			className: "ClientConnectionManager"
		};
	}

	/**
	 * Singleton init
	 */
	public init(): void {
	}

	/**
	 * Singleton exit
	 */
	public exit(): void {
	}

	/**
	 * Creates the client connection object, adds it to the list and sends the notify
	 * Is called from the express upgrade path after creating the WebSocket.Server object
	 *
	 * @param args - Arguments to construct the client connection
	 * @returns the created client connection object
	 */
	public createClientConnection(args: IClientConnectionConstructorArguments): ClientConnection {
		const con = new ClientConnection(args);
		con.on("destroyed", this.onEvent_ClientConnectionDestroyed);
		this.addConnection(con);
		this.fire_OnClientConnected(con);
		return con;
	}

	/**
	 * Connection destroyed event coming from the clientconnection itself
	 *
	 * @param con - The clientConnection object beeing destroyed
	 */
	public onEvent_ClientConnectionDestroyed(con: ClientConnection): void {
		this.removeConnection(con.clientConnectionID);
	}

	/**
	 * Adds the client connection to the internal list of connections
	 *
	 * @param con - The clientConnection object
	 */
	public addConnection(con: ClientConnection): void {
		this.clientConnections.set(con.clientConnectionID, con);
	}

	/**
	 * Removes the client connection from  the internal list of connections
	 *
	 * @param sessionID - The clientConnections sessionid to remove
	 * @returns true if the connectoin was found and removed
	 */
	public removeConnection(sessionID: string): boolean {
		theLogger.info("removing client connection", "removeConnection", this, { key: sessionID });
		const con = this.clientConnections.get(sessionID);
		if (con) {
			this.clientConnections.delete(sessionID);
			this.fire_OnClientDisconnected(con);
			return true;
		} else
			theLogger.error("client connection not found", "removeConnection", this, { key: sessionID });

		return false;
	}

	/**
	 * Fetches a client connection from the internal list of connections
	 *
	 * @param sessionID - The clientConnections sessionid to fetch
	 * @returns the ClientConnection if found in the list
	 */
	public get(sessionID: string): ClientConnection | undefined {
		return this.clientConnections.get(sessionID);
	}

	/**
	 * Fetches a client connection and returns the interface as required by the snacc sub
	 *
	 * @param id - The clientConnections sessionid to fetch
	 * @returns the ClientConnections IASN1ClientConnection interface if found in the list
	 */
	public getClientConnection(id: string): ClientConnection | undefined {
		return this.clientConnections.get(id);
	}

	/**
	 * Retrieves the amount of connected clients
	 *
	 * @returns the current amount of connected clients
	 */
	public getConnectionCount(): number {
		return this.clientConnections.size;
	}

	/**
	 * Retrieves all currently connected websocket clients
	 *
	 * @returns the ids of all the connected clients
	 */
	public getClientConnectionIDs(): string[] {
		return Array.from(this.clientConnections.keys());
	}

	/**
	 * Notifies that a client connected the server
	 *
	 * @param con - the connected client connection object
	 */
	public fire_OnClientConnected(con: ClientConnection): void {
		for (const notify of this.connectionNotifies)
			notify.on_ClientConnected(con);
	}

	/**
	 * Notifies that a client disconnected from the  server
	 *
	 * @param con - the disconnected client connection object
	 */
	public fire_OnClientDisconnected(con: ClientConnection): void {
		for (const notify of this.connectionNotifies)
			notify.on_ClientDisconnected(con);
	}

	/**
	 * Adds a notify receiver to the notifiers list of this class
	 *
	 * @param callback - the callback interface that shall be notified
	 */
	public addNotify(callback: IClientConnectionNotify): void {
		if (!this.connectionNotifies.has(callback))
			this.connectionNotifies.add(callback);
	}

	/**
	 * Revmoes a notify receiver from the notifiers list of this class
	 *
	 * @param callback - the callback interface that shall be removed
	 */
	public removeNotify(callback: IClientConnectionNotify): void {
		if (this.connectionNotifies.has(callback))
			this.connectionNotifies.delete(callback);
	}
}
