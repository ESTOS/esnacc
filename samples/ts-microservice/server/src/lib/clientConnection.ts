import { EventEmitter } from "events";
import { ILogData } from "uclogger";
import WebSocket from "ws";

import { IClientConnection, IClientConnectionConstructorArguments, ICustomReceiveInvokeContext } from "./IClientConnection";
import { IClientDetails } from "./IClientDetails";
import { ClientConnectionNotifies } from "./clientConnectionNotify";
import { Common } from "./common";
import { EOwnTimeout, EOwnInterval } from "./common_timers";
import { theConfig, theServer, theLogger, theLogStorage } from "../globals";
import { ILogContextStaticData, LogContextStaticData } from "../singletons/asyncLocalStorage";
import { EASN1TransportEncoding, ReceiveInvokeContext } from "../stub/TSROSEBase";

/**
 * A Helper class to get the RTT times of a client
 */
class RTTPingPongHelper {
	// Incremented ping pong counter which is added to the ping to associate the pong with the ping
	private static pingPongCounter = 0;
	// Our internal unique id
	public readonly id: number;
	// The associated ClientConnection object
	private readonly con: ClientConnection;
	// The timeout object which is called if we receive no pong
	private timeout: EOwnTimeout | undefined;
	// Time the object was created (ping sent)
	private time: number | undefined;

	/**
	 * Constructs the RTT Helper object
	 *
	 * @param con - The associated client connection object
	 */
	public constructor(con: ClientConnection) {
		this.id = ++RTTPingPongHelper.pingPongCounter;
		this.con = con;
		if (!con.wsClient)
			theLogger.error("websocket died", "constructor", this);
		else {
			// If the timeout does not come after 5 second we call it timedout
			this.timeout = new EOwnTimeout(this.onTimeout.bind(this), 5000);
			con.wsClient.ping(`${this.id}`);
			this.time = new Date().getTime();
		}
	}

	/**
	 * Cleanup the timer out
	 */
	public cleanUp(): void {
		if (this.timeout) {
			this.timeout.clearTimeout();
			this.timeout = undefined;
		}
	}

	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 *
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public getLogData(): ILogData {
		return {
			className: this.constructor.name
		};
	}

	/**
	 * If the pong was not handled within 5000 msec this function is called
	 */
	public onTimeout(): void {
		this.timeout = undefined;
		this.con.onPingPongTimeout(this.id);
	}

	/**
	 * Retrieves the RTT (current time vs. ping sent)
	 *
	 * @returns - the RTT or undefined if no ping was sent
	 */
	public getRTT(): number | undefined {
		if (!this.time)
			return undefined;
		return new Date().getTime() - this.time;
	}
}

/**
 * Each client is represented by a ClientConnection
 * The connection is created as soon as the websocket was established
 * For single shot rest requests no client connection object is created
 */
export class ClientConnection extends EventEmitter implements IClientConnection {
	// These are the details of the client we query while the client is connecting us
	public readonly clientDetails: IClientDetails;
	// The connection ID we assign to this connection
	public readonly clientConnectionID: string = "WS_" + Common.generateGUID();
	// The websocket object
	public wsClient?: WebSocket;

	// This data is added to the theLogger.logStorage for every inbound invoke from a client
	// The data is enriched as soon as we have another property to add here
	private logContext: LogContextStaticData;

	// Notifies for events of the client connection
	private notifies = new ClientConnectionNotifies();

	// *** These values are for the client triggered keepalive mechanism (client sends keepalive) ***
	// Timeout we use for the asnConferenceKeepAlive message
	private keepAliveTimeOut?: EOwnTimeout;

	// *** These values are for the server triggered keepalive mechanism (websocket ping pong) ***
	// Interval that is sending ping messages to the client
	private pingPongInterval?: EOwnInterval;

	// Stores the last round trip time the server evaluates every seconds with a websocket ping pong
	private rtt: number | undefined;

	// PingPong Helper map that allows us to handle multiple open ping objects at a time
	private running_rtt_tests = new Map<number, RTTPingPongHelper>();

	// The encoding this connection is using
	private encoding: EASN1TransportEncoding | undefined;

	// Every request that leads to an non zero result value increments this counter
	// The counter is decremented every second.
	// If the counter exceeds a certain value the connection is dropped.
	private errorCounter = 0;
	// Enabled/Disables logging for high errorCounter rates
	private errorCounterLogActive = true;
	// Interval that is decrementing the error counter
	private errorCounterDecrementInterval?: EOwnInterval;

	/** Acess to addNotify from the notifies class */
	public addNotify = this.notifies.addNotify.bind(this.notifies);
	/** Acess to removeNotify from the notifies class */
	public removeNotify = this.notifies.removeNotify.bind(this.notifies);

	/**
	 * Constructs the clientconnection object
	 *
	 * @param args - Arguments to construct the client connection
	 */
	public constructor(args: IClientConnectionConstructorArguments) {
		super();

		const forwarded = args.request.headers["x-forwarded-for"];
		let clientIP: string;
		if (forwarded && typeof forwarded === "string")
			clientIP = forwarded;
		else if (args.request.socket.remoteAddress)
			clientIP = args.request.socket.remoteAddress;
		else
			clientIP = "unknown";

		this.clientDetails = {
			clientIP
		};

		this.logContext = new LogContextStaticData({
			clientConnectionID: this.clientConnectionID
		});

		this.wsClientClose = this.wsClientClose.bind(this);
		this.wsClientMessage = this.wsClientMessage.bind(this);
		this.wsClientError = this.wsClientError.bind(this);
		this.wsClientPong = this.wsClientPong.bind(this);
		this.onKeepAliveTimeOut = this.onKeepAliveTimeOut.bind(this);
		this.onPingPongInterval = this.onPingPongInterval.bind(this);
		this.onErrorCounterDecrement = this.onErrorCounterDecrement.bind(this);

		theLogger.debug(`ClientConnection created`, "constructor", this);
		if (theConfig.debugObjectCreationDestruction)
			(console as Console).log(`+ ClientConnection created ${this.clientConnectionID}`);
	}

	/**
	 * Set the websocket that we received after the verifyClient method has been processed
	 *
	 * @param ws - the inbound client websocket connection object
	 */
	public setWebSocket(ws: WebSocket): void {
		this.wsClient = ws;
		this.init();
	}

	/**
	 * Initializes the clientConnection object
	 */
	public init(): void {
		this.setPingPongInterval(1000);
		this.setErrorCounterDecrementInterval(1000);
		if (this.wsClient) {
			this.wsClient.addListener("error", this.wsClientError);
			this.wsClient.addListener("message", this.wsClientMessage);
			this.wsClient.addListener("close", this.wsClientClose);
			this.wsClient.addListener("pong", this.wsClientPong);
		}
	}

	/**
	 * Cleans up the clientConnection object
	 */
	public cleanUp(): void {
		for (const test of this.running_rtt_tests.values())
			test.cleanUp();
		this.setKeepAliveTimeout(0);
		this.setPingPongInterval(0);
		this.setErrorCounterDecrementInterval(0);
		if (this.wsClient) {
			this.wsClient.removeAllListeners();
			this.wsClient.close();
			this.wsClient = undefined;
		}
		theLogger.debug(`ClientConnection destroyed`, "cleanUp", this);
		if (theConfig.debugObjectCreationDestruction)
			(console as Console).log(`- ClientConnection destroyed ${this.clientConnectionID}`);
	}

	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 *
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public getLogData(): ILogData {
		return {
			className: this.constructor.name,
			classProps: {
				...this.logContext
			}
		};
	}

	/**
	 * Retrieves the current round trip time which is evaluated every 5 seconds.
	 * The server sends a websocket ping and waits for the response
	 * The rtt is logged
	 *
	 * @returns - The websocket round trip time in msec or undefined if it has not yet been evaluated
	 */
	public getRTT(): number | undefined {
		return this.rtt;
	}

	/**
	 * Sending data to connected client over websocket connection
	 *
	 * @param data - data send to client
	 * @returns - true on success or false on error
	 */
	public send(data: string | ArrayBuffer): boolean {
		if (!this.wsClient || this.wsClient.readyState !== this.wsClient.OPEN)
			return false;

		this.wsClient.send(data);
		return true;
	}

	/**
	 * This message is called for every keepAlive message the conferenceController receives
	 * If no keepalive is received the connection will timeout and discarded in onKeepAliveTimeOut
	 */
	public resetKeepAliveTimeout(): void {
		this.setKeepAliveTimeout(theConfig.clientConnection.client_keepalive);
	}

	/**
	 * Logs websocket errors to our logging
	 *
	 * @param error - the websockets error object
	 */
	public wsClientError(error: Error): void {
		theLogger.warn("websocket error received", "wsClientError", this, { error });
	}

	/**
	 * If the websocket is closed we handle it here
	 *
	 * @param code - the closing reason
	 * @param message - the closing message
	 */
	public wsClientClose(code: number, message: WebSocket.Data): void {
		theLogger.info("websocket was closed", "wsClientClose", this, { code, message: message.toString(), clientDetails: this.clientDetails });
		this.cleanUp();
		this.emit("destroyed", this);
	}

	/**
	 * Called when data is received from client
	 *
	 * @param data - data send from client
	 * @param isBinary - data is binary
	 */
	public async wsClientMessage(data: WebSocket.RawData, isBinary: boolean): Promise<void> {
		const customData: ICustomReceiveInvokeContext = {
			logContext: this.logContext,
			clientDetails: this.clientDetails
		};
		const invokeContext = new ReceiveInvokeContext({
			clientConnectionID: this.clientConnectionID,
			clientIP: this.clientDetails.clientIP,
			encoding: this.encoding,
			customData
		});

		// Set the logContext for our invoke to have it available everywhere we then go within our code
		theLogStorage.enterWith(this.logContext);

		let message: Uint8Array | undefined;
		if (data instanceof Uint8Array)
			message = data;
		else if (data instanceof ArrayBuffer)
			message = new Uint8Array(data);
		else if (data instanceof Blob)
			message = new Uint8Array(await data.arrayBuffer());
		else {
			theLogger.error("Received payload which we could not handle", "wsClientMessage", this);
			return;
		}

		try {
			const result = await theServer.receive(message, invokeContext);
			if (this.encoding === undefined)
				this.encoding = invokeContext.encoding;
			else if (this.encoding !== invokeContext.encoding) {
				debugger;
				theLogger.error("Client wants to change encoding on a websocket connection", "wsClientMessage", this, { encoding: this.encoding, invokeContext });
			}
			if (result) {
				this.send(result.payLoad);
				if (result.httpStatusCode && result.httpStatusCode !== 200 && this.errorCounterLogActive) {
					this.errorCounter++;
					if (this.errorCounter > 10) {
						this.errorCounterLogActive = false;
						theLogger.error("multiple errors occured, dropping client connection", "wsClientMessage", this);
					}
				}
			}
		} catch (err) {
			theLogger.error("exception theServer.receive", "wsClientMessage", this, undefined, err);
		}
	}

	/**
	 * Updates elements in the logContext
	 *
	 * @param data - attributes to set into the logContext
	 * @returns true if the logContext has changed
	 */
	public updateLogContext(data: Partial<ILogContextStaticData>): boolean {
		let bChanged = false;
		if (data.clientID !== undefined && data.clientID !== this.logContext.clientID) {
			this.logContext.clientID = data.clientID;
			bChanged = true;
		}
		if (data.clientConnectionID !== undefined && data.clientConnectionID !== this.logContext.clientConnectionID) {
			this.logContext.clientConnectionID = data.clientConnectionID;
			bChanged = true;
		}
		if (bChanged)
			theLogStorage.enterWith(this.logContext);
		return bChanged;
	}

	/**
	 * The connection to the client is monitored
	 * The client must send keepAlive message within the specified amount of time
	 *
	 * This method is for the client triggered keepalive mechanism
	 *
	 * @param timeout - after timeout msec the client connection is discarded
	 */
	private setKeepAliveTimeout(timeout: number): void {
		if (this.keepAliveTimeOut) {
			this.keepAliveTimeOut.clearTimeout();
			this.keepAliveTimeOut = undefined;
		}
		if (timeout)
			this.keepAliveTimeOut = new EOwnTimeout(this.onKeepAliveTimeOut, timeout);
	}

	/**
	 * If the client did not send keepAlive messages the connection is now discarded
	 *
	 * This method is for the client triggered keepalive mechanism
	 */
	public onKeepAliveTimeOut(): void {
		theLogger.warn("timed out, dropping connection", "onKeepAliveTimeOut", this, { clientDetails: this.clientDetails });
		this.notifies.fire_On_ClientKeepAliveTimedOut(this);
		this.cleanUp();
		this.emit("destroyed", this);
	}

	/**
	 * Sets the interval in which the server sends websocket ping messages to the client
	 * If the ping is sent we start another timer that waits for the pong
	 * If the pong arrives in time everytbing is safe.
	 * If the pong does not arrive we have an error counter that releases the connection if three pongs got lost
	 *
	 * This method is for the server triggered keepalive mechanism
	 *
	 * @param interval - the interval in msec to be used or 0 to reset the interval
	 */
	private setPingPongInterval(interval: number): void {
		if (this.pingPongInterval) {
			this.pingPongInterval.clearInterval();
			this.pingPongInterval = undefined;
		}
		if (interval) {
			this.pingPongInterval = new EOwnInterval(this.onPingPongInterval, interval);
			this.onPingPongInterval();
		}
	}

	/**
	 * Sets the interval in which the error Counter is decremented if it is larger than 0
	 *
	 * @param interval - the interval in msec to be used or 0 to reset the interval
	 */
	private setErrorCounterDecrementInterval(interval: number): void {
		if (this.errorCounterDecrementInterval) {
			this.errorCounterDecrementInterval.clearInterval();
			this.errorCounterDecrementInterval = undefined;
		}
		if (interval)
			this.errorCounterDecrementInterval = new EOwnInterval(this.onErrorCounterDecrement, interval);
	}

	/**
	 * This method sends the ping to the client and starts the pong timeout timer
	 *
	 * This method is for the server triggered keepalive mechanism
	 */
	private onPingPongInterval(): void {
		if (this.wsClient) {
			const ping = new RTTPingPongHelper(this);
			this.running_rtt_tests.set(ping.id, ping);
		}
	}

	/**
	 * Is called by the RTTPingPongHelper object if the pong was not received
	 *
	 * @param id - id of the ping pong object
	 */
	public onPingPongTimeout(id: number): void {
		const pingPong = this.running_rtt_tests.get(id);
		if (pingPong) {
			pingPong.cleanUp();
			this.running_rtt_tests.delete(id);
			theLogger.warn("pong timed out", "onPingPongTimeout", this, { id });
		}
	}

	/**
	 * The clients pong is handled here
	 * We reset the pongtimeout and reset the error counter value
	 *
	 * This method is for the server triggered keepalive mechanism
	 *
	 * @param data - the data received with the pong
	 */
	private wsClientPong(data: Buffer): void {
		if (!data) {
			theLogger.warn("received a pong without payload", "wsClientPong", this);
			return;
		}
		const id = parseInt(data.toString());
		const pingPong = this.running_rtt_tests.get(id);
		if (pingPong) {
			const rtt = pingPong.getRTT();
			pingPong.cleanUp();
			this.running_rtt_tests.delete(id);
			if (rtt !== undefined) {
				if (this.rtt !== rtt) {
					this.rtt = rtt;
					this.notifies.fire_On_RTTChanged(this, rtt);
				}
			}
		}
	}

	/**
	 * This method decrements the error counter every time it is called if the value is greater than 0
	 */
	private onErrorCounterDecrement(): void {
		if (this.errorCounter > 0)
			this.errorCounter--;
	}

	/**
	 * Notifies that a client is disconnecting from the server
	 * The notify has a slightly different name than the IClientConnectionManagerNotify as this event comes prior to the on_ClientDisconnected
	 * While this notify is processed the client is still in the list of connections in the clientConnectionManager
	 */
	public fire_On_ClientDisconnecting(): void {
		this.notifies.fire_On_ClientDisconnecting(this);
	}

	/**
	 * Returns the encoding this clientConnection has been setup with
	 *
	 * @returns the encoding of the connection
	 */
	public getTransportEncoding(): EASN1TransportEncoding | undefined {
		return this.encoding;
	}
}

/**
 * Map of client connections, key is a random GUID created in the client connection
 */
export class ClientConnections extends Map<string, ClientConnection> {
}
