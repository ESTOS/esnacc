import { IncomingMessage } from "node:http";
import WebSocket from "ws";

import { IClientDetails } from "./IClientDetails.js";
import { ILogContextData, ILogContextStaticData } from "../singletons/asyncLocalStorage.js";
import { IASN1ClientConnection } from "../stub/TSASN1Server.js";

/**
 * Custom Invoke Data we add to the receive invoke context as customData (void*)
 */
export interface ICustomReceiveInvokeContext {
	// The log context object we store in the clientConnection
	// ! This is ref counted by javascript so 1:1 the same as in the clientConnection
	// ! If we change it we adopt the one in the clientConnection accordingly (wanted behaviour)
	logContext: ILogContextData;
	// These are the details of the client we query while the client is connecting us
	clientDetails: IClientDetails;
}

export interface IClientConnectionConstructorArguments {
	request: IncomingMessage;
}

/**
 * Interface to the client connection object
 */
export interface IClientConnection extends IASN1ClientConnection {
	readonly clientDetails: IClientDetails;
	readonly clientConnectionID: string;
	wsClient?: WebSocket;
	setWebSocket(ws: WebSocket): void;
	addNotify(callback: IClientConnectionNotify): void;
	removeNotify(callback: IClientConnectionNotify): void;
	updateLogContext(data: Partial<ILogContextStaticData>): void;
	resetKeepAliveTimeout(): void;
}

/**
 * Interface a client connection subscriber needs to implement
 */
export interface IClientConnectionNotify {
	// Readable notify name for debugging purposes
	readonly notifyName: string;

	/**
	 * Notifies that the client connection has a new rtt (round trip time value)
	 * @param con - the affected client connection
	 * @param rtt - the new round trip time in msec
	 */
	on_RTTChanged?(con: IClientConnection, rtt: number): void;

	/**
	 * Notifies that a client is disconnecting from the server
	 * The notify has a slightly different name than the IClientConnectionManagerNotify as this event comes prior to the on_ClientDisconnected
	 * While this notify is processed the client is still in the list of connections in the clientConnectionManager
	 * @param con - the connected client connection object
	 */
	on_ClientDisconnecting?(con: IClientConnection): void;

	/**
	 * Notifies that a client timed out in sending a keepalive to the server
	 * @param con - the connected client connection object
	 */
	on_ClientKeepAliveTimedOut?(con: IClientConnection): void;
}

/**
 * The notify interface someone that wants to subscribe to the clientConnectionManager via addNotify has to implement
 */
export interface IClientConnectionManagerNotify {
	// Readable notify name for debugging purposes
	readonly notifyName: string;

	/**
	 * Notifies that a client connected the server
	 * @param con - the connected client connection object
	 */
	on_ClientConnected?(con: IClientConnection): void;
	/**
	 * Notifies that a client disconnected from the server
	 * @param con - the connected client connection object
	 */
	on_ClientDisconnected?(con: IClientConnection): void;
}
