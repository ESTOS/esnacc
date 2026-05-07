// Centralised code for the TypeScript converters.
// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do not directly edit or modify the code as it is machine generated and will be overwritten with every compilation

// dprint-ignore-file
/* eslint-disable */

import * as ENetUC_Common from "./ENetUC_Common.js";
import { ROSEError, ROSEReject, ROSEResult } from "./SNACCROSE.js";
import { ASN1ClassInstanceType, IResponseData, PendingInvoke, TSASN1Base } from "./TSASN1Base.js";
import { EASN1TransportEncoding } from "./TSInvokeContext.js";
import {
	createInvokeReject,
	CustomInvokeProblemEnum,
	EHttpHeaders,
	ELogSeverity,
	ESocketState,
	IASN1InvokeData,
	IASN1Transport,
	IConnectionSocket,
	ISendInvokeContext,
	ISocketCloseEvent,
	ISocketConnectedEvent,
	ISocketErrorEvent,
	ISocketMessageEvent,
	ReceiveInvokeContext,
	ROSEBase,
} from "./TSROSEBase.js";

export interface IWebSocketOptions {
	perMessageDeflate?: boolean;
	headers?: { [key: string]: string; };
}

// Node is not aware of this type but node-fetch which we use until node itself supports fetch uses it that way
type HeadersInit = string[][] | Record<string, string> | Headers;

/**
 * A Promise that is fullfilled if the requested websocket was created or rejected if the creation failed
 */
class ConnectionSocketPromise {
	public readonly resolve: (value: IConnectionSocket) => void;
	public readonly reject: (reason: ENetUC_Common.AsnRequestError) => void;
	/**
	 * Constructs a websocket promise object, simply stores the handed over arguments in the class
	 *
	 * @param resolve - The resolve method that is called if the websocket was created
	 * @param reject - The rejcet method that is called if something unhandled did occur
	 */
	public constructor(
		resolve: (value: IConnectionSocket) => void,
		reject: (reason: ENetUC_Common.AsnRequestError) => void,
	) {
		this.resolve = resolve;
		this.reject = reject;
	}
}

/*
 An array of promises that is waiting for a connection setup
 */
class PendingSockets
{
	private promises = new Array<ConnectionSocketPromise>();

	/**
	 * Adds another pending socket promise to the list of pending sockets
	 *
	 * @param promise - The pending promise
	 */
	public push(promise: ConnectionSocketPromise) {
		this.promises.push(promise);
	}

	/**
	 * Resolve the pending promises with a connected socket
	 *
	 * @param connection - the connected socket
	 */
	public resolve(connection: IConnectionSocket) {
		// Handle all pending operations
		for (const pending of this.promises)
			pending.resolve(connection);
		this.promises = [];
	}

	/**
	 * In case of a statefull connection error we fullfill all pending sockets with the error object
	 *
	 * @param error - The error object as provided by the caller
	 */
	public reject(error: unknown): void {
		let err: ENetUC_Common.AsnRequestError;
		if (error instanceof ENetUC_Common.AsnRequestError)
			err = error;
		else if (error instanceof Error) {
			err = new ENetUC_Common.AsnRequestError({
				iErrorDetail: CustomInvokeProblemEnum.serviceUnavailable,
				u8sErrorString: error.message,
			});
		}
		else {
			const erroMsg = error instanceof Object ? JSON.stringify(error, null, 2) : String(error);
			err = new ENetUC_Common.AsnRequestError({
				iErrorDetail: CustomInvokeProblemEnum.serviceUnavailable,
				u8sErrorString: `Unknown error received: ${erroMsg}`,
			});
		}
		for (const pendingSocket of this.promises)
			pendingSocket.reject(err);
		this.promises = [];
	}
}

/**
 * Connection related callbacks if we get connected or disconnected
 */
export interface IClientConnectionCallback {
	// Is called in case the client is not connected and about to establish a statefull connection
	// Within that callback the target may get changed (e.g. if you need auth data for the websocket connection)
	// Returning false will terminate the connect request (and all pending connect request)
	onBeforeConnect?(bReconnecting: boolean): Promise<boolean>;
	// Is called after the client has opened the websocket connection
	onClientConnected(bReconnected: boolean): Promise<void>;
	// Is called after the (established) websocket connection did disconnect
	onClientDisconnected(bControlledDisconnect: boolean): Promise<void>;
}

/**
 * The connection mode the client is currently using
 */
export enum EASNCONNECTIONMODE {
	// Unknown connection mode (default)
	UNKNOWN = 0,
	// Websocket connection (target points to ws or wss)
	WEBSOCKET = 1,
	// REST connection (target points to http or https)
	REST = 2,
	// TCP connection (target points to tcp),
	TCP = 3,
}

/**
 * The TSASN1Client implements the ROSE client side
 * It takes care of all the related rose handlings
 *
 * - establishing a connection to the target
 * - providing the transport layer for the invoke messages
 * -   encapsulating of messages (arguments and rose part)
 * -   transport of messages (completion objects with own cue, timeout handling)
 * -   association of result messages to the invoke
 * - logging
 */
export abstract class TSASN1Client extends TSASN1Base implements IASN1Transport {
	// Connection target the client is connecting to
	// If target points to:
	// ws(s) a websocket connection is established
	// http(s) the Object is calling the server using POST requests (message in body)
	protected target = "";
	protected connectionMode = EASNCONNECTIONMODE.UNKNOWN;

	// The client side socket (websocket or raw tcp socket) towards the server
	// Only beeing used if the connection is statefull using websockets or raw sockets (target points to ws, wss or tcp)
	private socket?: IConnectionSocket;

	// These Properties are ONLY used if the client is using websockets to connect to the server:
	// -------------------------------------
	// The client automatically tries to reconnect the target if the connection was dropped
	protected autoReconnect = true;
	// The sessionID the server provided for this client (has to be filled by the businesslogik as soon as a session id is available)
	protected clientConnectionID?: string;
	// List of pending requests for a sockets towards the target
	// As long as the connection is beeing established every request for a socket is queued
	// This list is automatically cleared if the connection succeeds or fails.
	protected pendingSockets = new PendingSockets();

	// We are currently reconnecting
	private reconnecting = false;
	// Helper to parametrise the reconnect timer in case of connection failures (first 10 approaces retry every second, afterwards every 5 seconds)
	private reconnectCounter = 0;
	// We are currently opening a statefull connection (asynchronous function)
	private openingStateFullConnection = false;
	// Connection related callback list for the ones that are interested in it (add removeConnectionCallback)
	private connectionCallBack = new Set<IClientConnectionCallback>();

	// Additional headers we want to pass with every fetch and websocket init request towards the server
	private additionalHeaders?: EHttpHeaders;

	/**
	 * Constructs the Client and binds some messages to this.
	 *
	 * @param encoding - sets the encoding for outbound calls
	 * @param instanceType - the type of instance this class belongs to
	 */
	protected constructor(
		encoding: EASN1TransportEncoding.JSON | EASN1TransportEncoding.BER,
		instanceType: ASN1ClassInstanceType,
	) {
		super(encoding, instanceType);
		this.onSocketClose = this.onSocketClose.bind(this);
		this.onSocketMessage = this.onSocketMessage.bind(this);
		this.onSocketError = this.onSocketError.bind(this);
	}

	/**
	 * Helper method that provides the socket state as text for logging and debugging
	 *
	 * @param state - The state the method should provide as string
	 * @returns - the state as text or UNKNOWN if an unknown state was provided
	 */
	public static getWebSocketReadyStateAsString(state: ESocketState): string {
		switch (state) {
			case ESocketState.CONNECTING:
				return "CONNECTING";
			case ESocketState.OPEN:
				return "OPEN";
			case ESocketState.CLOSING:
				return "CLOSING";
			case ESocketState.CLOSED:
				return "CLOSED";
			default:
				debugger;
				return "UNKNOWN";
		}
	}

	/**
	 * Returns the ready state of the websocket if the websocket has been initalized, otherwise undefined
	 *
	 * @returns - the websocket readyState
	 */
	public getSocketState(): ESocketState | undefined {
		if (!this.socket)
			return undefined;
		return this.socket.readyState;
	}

	/**
	 * Retrieves the client connection ID we got from the server
	 *
	 * @returns the client connection ID (ROSE sessionID)
	 */
	public getSessionID(): string | undefined {
		return this.clientConnectionID;
	}

	/**
	 * Sends an event synchronous to the server
	 *
	 * @param data - the data for the invoke
	 * @returns true when the event was sent
	 */
	public sendEventSync(data: IASN1InvokeData): boolean {
		if (this.socket && this.socket.readyState === ESocketState.OPEN) {
			const encodeResult = ROSEBase.encodeToTransport(data.payLoad, this.encodeContext);
			this.socket.send(encodeResult.payLoad);
			return true;
		}
		return false;
	}

	/**
	 * Sends a message towards the server
	 * Creates a completion object to associate request with response.
	 * The callback is handed over to the completion object and called out of it (see above)
	 * For and event no timeout or callback is used or required (invokeID already set to 99999 for an event while calling)
	 *
	 * @param data - the data for the invoke
	 * @returns - a Promise based ROSE message (reject, result, error) as provided by the other side (or in case of timeout).
	 * If no timeout was specified we resolve in undefined to cleanup the promise object
	 */
	public async sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		return new Promise((resolve): void => {
			let resolveUndefined = true;

			let connectionMode = this.connectionMode;

			// Allows to specify a REST target through the invokeContext for this request (indipendently from the this.target)
			if (data.invokeContext?.restTarget)
				connectionMode = EASNCONNECTIONMODE.REST;

			// If this is an invoke
			if (data.invokeContext.invokeID !== 99999) {
				const timeout = data.invokeContext?.timeout || this.defaultTimeout;
				// Create a completion object if:
				// - a timeout was provided
				// - a rest request is processed (we wait until the request has been handled)
				//   We handle the rest result through the same methods as the websocket so we
				//   need this completion object as the object is completed in those handling methods
				if (connectionMode === EASNCONNECTIONMODE.REST || timeout) {
					const id = data.invokeContext.invokeID;
					let timerID: ReturnType<typeof setTimeout> | undefined;
					if (timeout) {
						timerID = setTimeout((): void => {
							this.onROSETimeout(id);
						}, timeout);
					}
					const pending = new PendingInvoke(data.invoke, resolve, timerID);
					this.pendingInvokes.set(id, pending);
					resolveUndefined = false;
				}
			}
			if (connectionMode === EASNCONNECTIONMODE.WEBSOCKET || connectionMode === EASNCONNECTIONMODE.TCP)
				this.sendStatefull(data);
			else if (connectionMode === EASNCONNECTIONMODE.REST)
				this.sendStateLess(data);
			else if (this.target === ``)
				throw new Error(`You need to specify a connection target either through setTarget() or the ISendInvokeContext`);
			else
				throw new Error(`Unsupported target ${this.target}`);
			if (resolveUndefined)
				resolve(undefined);
		});
	}

	/**
	 * Sends data via a statefull socket connection
	 * - Connects to the target if not already connected
	 * - Handles errors
	 * - Sends data
	 *
	 * @param data - the data object which contains all the information
	 */
	private sendStatefull(data: IASN1InvokeData): void {
		// Get or create a connection to the target
		this.getConnection(data.invokeContext).then((connection: IConnectionSocket): void => {
			const encodeResult = ROSEBase.encodeToTransport(data.payLoad, this.encodeContext);
			this.logTransport(encodeResult.logData, "sendStatefull", "out", data.invokeContext);
			// Send the message
			connection.send(encodeResult.payLoad);
		}).catch((error: unknown): void => {
			// Could not connect to the target or an unknown error occured
			let invokeReject: ROSEReject;
			if (error instanceof ENetUC_Common.AsnRequestError)
				invokeReject = createInvokeReject(data.invoke, error.iErrorDetail, error.u8sErrorString);
			else {
				invokeReject = createInvokeReject(
					data.invoke,
					CustomInvokeProblemEnum.serviceUnavailable,
					`Could not connect to ${this.target}`,
				);
			}
			// Handl rose reject if we did not successfully connect to the target
			this.log(ELogSeverity.error, "Could not establish connection", "sendWebSocket", this, {
				invokeContext: data.invokeContext,
			}, error);

			// Reject the currently pending operation
			const receiveInvokeContext = ReceiveInvokeContext.create(data.invoke);
			this.onROSEReject(invokeReject, receiveInvokeContext);
		});
	}

	/**
	 * Does the async fetch and handles all errors that might occur
	 *
	 * @param target - the target we are connecting to
	 * @param data - the data object which contains all the information
	 */
	private async handleFetch(target: string, data: IASN1InvokeData): Promise<void> {
		const encoding = data.invokeContext.encoding;
		const headers: HeadersInit = {
			"Content-Type": encoding === EASN1TransportEncoding.JSON ? "application/json" : "application/octet-stream",
		};
	
		const encodeResult = ROSEBase.encodeToTransport(data.payLoad, this.encodeContext);

		// Contains the encoded data for the transport
		const body = encodeResult.payLoad;

		// Set the additional headers from the class object
		if (this.additionalHeaders) {
			const keys = Object.keys(this.additionalHeaders);
			for (const key of keys) {
				const obj = this.additionalHeaders[key];
				if (obj && typeof obj === "string")
					headers[key] = obj;
			}
		}

		// Set the additional headers from the invoke object
		if (data.invokeContext?.headers) {
			const keys = Object.keys(data.invokeContext.headers);
			for (const key of keys) {
				const obj = data.invokeContext.headers[key];
				if (obj && typeof obj === "string")
					headers[key] = obj;
			}
		}

		// We can either send the request with ROSE envelop or without (not really needed here), default is without

		// Build the http request object

		const requestdata: RequestInit = {
			method: "POST",
			body: body instanceof Uint8Array ? new Uint8Array(body) : body,
			headers,
		};

		// REST requests are handled through the pending invokes list
		// Every request is added to this list no matter if we defined a timeout or not
		// So we do not directly see the handling of the result.
		// The result is handled through the regular methods and completed in the background
		this.logTransport(encodeResult.logData, "sendInvoke", "out", data.invokeContext);
		const receiveInvokeContext = ReceiveInvokeContext.create(data.invoke);

		const response = await this.fetch(target, requestdata);
		try {
			// We received a result (Will this work for BER encoding as well?)
			let message: Uint8Array | object;
			if (encoding === EASN1TransportEncoding.BER) {
				const buffer = await response.arrayBuffer();
				message = new Uint8Array(buffer);
			}
			else {
				message = (await response.json()) as object;
			}
			await this.receive(message, receiveInvokeContext);
		}
		catch (error: unknown) {
			this.log(ELogSeverity.error, "receiving failed with exception", "sendInvoke", this, undefined, error);
			const reject = createInvokeReject(data.invoke, response.status, response.statusText);
			this.onROSEReject(reject, receiveInvokeContext);
		}
	}

	/**
	 * Sends a single request via a REST fetch connection
	 * - Connects via fetch
	 * - Handles errors
	 * - Sends data
	 *
	 * @param data - the data object which contains all the information
	 */
	private sendStateLess(data: IASN1InvokeData): void {
		// Allows to specify a REST target through the invokeContext for this request (indipendently from the this.target)
		let target = data.invokeContext?.restTarget || this.target;
		// Add the method we are calling to the url
		if (target.charAt(target.length - 1) !== "/")
			target += "/";
		target += data.invokeContext.operationName;

		this.handleFetch(target, data).catch((error: unknown) => {
			this.log(ELogSeverity.error, "Could not establish connection", "sendInvoke", this, undefined, error);
			const reject = createInvokeReject(
				data.invoke,
				CustomInvokeProblemEnum.serviceUnavailable,
				`Could not connect to ${target}`,
			);
			const receiveInvokeContext = ReceiveInvokeContext.create(data.invoke);
			this.onROSEReject(reject, receiveInvokeContext);
		});
	}

	/**
	 * Helper method that allows to dedicatedly call connect without sending something (test connection method)
	 *
	 * @param invokeContext - contextual data provided with the invoke
	 * @returns - a Promise based ROSE message (reject, result, error) as provided by the other side (or in case of timeout).
	 * If no timeout was specified we resolve in undefined to cleanup the promise object
	 */
	public async connect(invokeContext?: ISendInvokeContext): Promise<boolean> {
		switch (this.connectionMode) {
			case EASNCONNECTIONMODE.WEBSOCKET:
			case EASNCONNECTIONMODE.TCP:
				try {
					await this.getConnection(invokeContext);
					return true;
				}
				catch (error: unknown) {
					this.log(ELogSeverity.error, "exception", "connect", this, undefined, error);
				}
				break;
			case EASNCONNECTIONMODE.REST:
				this.log(ELogSeverity.warn, "REST connectionMode does not support connect", "connect", this, {
					connectionMode: this.connectionMode,
				});
				break;
			default:
				this.log(ELogSeverity.error, "Invalid connection mode", "connect", this, {
					connectionMode: this.connectionMode,
				});
				break;
		}
		return false;
	}

	/**
	 * Helper method to disconnect a websocket connection
	 *
	 * @param bControlledDisconnect - set to true if we do the shutdown in a controlled way (to differ between a disconnect not intented by us)
	 */
	public async disconnect(bControlledDisconnect: boolean): Promise<void> {
		this.shutdown("disconnect");
		await this.fire_OnDisconnected(bControlledDisconnect);
	}

	/**
	 * Adds a handler to the list of connection callbacks
	 *
	 * @param callback - The handler that wants to be informed about a connect, disconnect
	 */
	public addConnectionCallback(callback: IClientConnectionCallback): void {
		this.connectionCallBack.add(callback);
	}

	/**
	 * Removes a handler from the list of connection callbacks
	 *
	 * @param callback - The handler that wants to be informed about a connect, disconnect
	 */
	public removeConnectionCallback(callback: IClientConnectionCallback): void {
		if (this.connectionCallBack.has(callback))
			this.connectionCallBack.delete(callback);
	}

	/**
	 * Sets additional http header elements we want to pass with every fetch and websocket init request towards the server
	 *
	 * @param headers - The additional headers to set
	 */
	public setAdditionalHTTPHeaders(headers?: EHttpHeaders): void {
		this.additionalHeaders = headers;
	}

	/**
	 * Allows to dedicately set a connection target
	 *
	 * @param newTarget - The target we want to connect to. Prefix with ws or http to specify the connection mode
	 */
	public setTarget(newTarget: string): void {
		const oldTarget = this.target;
		try {
			if (this.target !== newTarget) {
				this.log(ELogSeverity.debug, "Setting new Target", "setTarget", this, { oldTarget, newTarget });
				this.target = newTarget;
				const targetlc = this.target.toLowerCase();
				if (targetlc.startsWith("ws"))
					this.connectionMode = EASNCONNECTIONMODE.WEBSOCKET;
				else if (targetlc.startsWith("http"))
					this.connectionMode = EASNCONNECTIONMODE.REST;
				else if (targetlc.startsWith("tcp"))
					this.connectionMode = EASNCONNECTIONMODE.TCP;
				else {
					this.connectionMode = EASNCONNECTIONMODE.UNKNOWN;
					if (newTarget.length) {
						throw new Error(
							`Unknown connection target protocol ${this.target}, expecting websocket, http rest like or tcp (ws,wss,http,https,tcp)`,
						);
					}
				}
				if (this.socket) {
					this.disconnect(true).then(() => {}).catch((error) => {
						this.log(ELogSeverity.error, "Failed to disconnect", "setTarget", this, { oldTarget, newTarget }, error);
					});
				}
			}
		}
		catch (error: unknown) {
			this.log(ELogSeverity.error, "Exception", "setTarget", this, { oldTarget, newTarget }, error);
			throw error;
		}
	}

	/**
	 * Allows to fetch the current connection target
	 *
	 * @returns - the current connection target
	 */
	public getTarget(): string {
		return this.target;
	}

	// These methods differ between node and browser implementation, thus we implement them in the approriate TSASN1BrowserClient and TSASN1NodeClient
	protected abstract getConnectionSocket(address: string, options?: IWebSocketOptions): IConnectionSocket | undefined;
	protected abstract fetch(input: string, init?: RequestInit): Promise<Response>;
	protected abstract setReconnectTimeout(timeout: number): void;

	/**
	 * Helper function to get or create a websocket connection object.
	 * Implements a single-flight pattern: if a connection attempt is already in progress,
	 * the caller is queued and resolved once the ongoing attempt completes.
	 *
	 * @param invokeContext - contextual data provided with the invoke
	 * @returns - An IConnectionSocket object on success
	 */
	private async getConnection(invokeContext?: ISendInvokeContext): Promise<IConnectionSocket> {
		if (this.socket)
			return this.socket;

		return new Promise((resolve, reject): void => {
			// Add the request to the pending list
			this.pendingSockets.push(new ConnectionSocketPromise(resolve, reject));
			if (!this.openingStateFullConnection) {
				this.createStateFullConnection(invokeContext).catch((error: unknown): void => {
					this.log(
						ELogSeverity.error,
						"createStateFullConnection exception catched",
						"getConnection",
						this,
						undefined,
						error,
					);
				});
			}
		});
	}
	/**
	 * Helper function to create a statefull connection object
	 *
	 * @param invokeContext - contextual data provided with the invoke
	 */
	private async createStateFullConnection(invokeContext?: ISendInvokeContext): Promise<boolean> {
		if (this.socket)
			return true;

		// Set the flag that we are creating a statefull connection (pending operation in progress)
		this.openingStateFullConnection = true;

		if (!await this.fire_OnBeforeConnect(this.reconnecting)) {
			throw new ENetUC_Common.AsnRequestError({
				iErrorDetail: CustomInvokeProblemEnum.serviceUnavailable,
				u8sErrorString: `Connecting terminated as a onBeforeReconnect returned false`,
			});
		}

		return new Promise((resolve): void => {
			// Set websocket options
			const options: IWebSocketOptions = { perMessageDeflate: false };

			// Set the additional headers from the class object
			if (this.additionalHeaders) {
				const keys = Object.keys(this.additionalHeaders);
				for (const key of keys) {
					let value = this.additionalHeaders[key];
					if (Array.isArray(value))
						value = value[0];
					if (value) {
						if (!options.headers)
							options.headers = {};
						options.headers[key] = value;
					}
				}
			}

			// Set the additional headers from the invoke object
			if (invokeContext?.headers) {
				const keys = Object.keys(invokeContext.headers);
				for (const key of keys) {
					let value = invokeContext.headers[key];
					if (Array.isArray(value))
						value = value[0];
					if (value) {
						if (!options.headers)
							options.headers = {};
						options.headers[key] = value;
					}
				}
			}

			const socket = this.getConnectionSocket(this.target, options);
			if (!socket)
				throw new Error("Failed to get a websocket object");
			/** called if the websocket was opend (connect to the target */
			socket.onSocketConnected = (): void => {
				// In case we are connected
				this.log(ELogSeverity.info, "Connected to", "createStateFullConnection", this, { target: this.target });

				// Map the event listener to the socket object
				socket.onSocketConnected = undefined;
				socket.onSocketMessage = this.onSocketMessage;
				socket.onSocketError = this.onSocketError;
				socket.onSocketClose = this.onSocketClose;

				// hold the socket, we need it to send data and close it again
				this.socket = socket;
				// Set our flags properly
				this.reconnecting = false;
				this.reconnectCounter = 0;

				// tell the notifies that we are connected
				this.fire_OnConnected(this.reconnecting).catch((error: unknown) => {
					this.log(
						ELogSeverity.error,
						"fire_OnConnected exception catched",
						"createStateFullConnection",
						this,
						undefined,
						error,
					);
				}).finally(() => {
					this.pendingSockets.resolve(socket);
					this.openingStateFullConnection = false;
					resolve(true);
				});
			};
			/**
			 * called if the websocket was closed (opening failed)
			 *
			 * @param closed - the id of the closing
			 */
			socket.onSocketClose = (closed: ISocketCloseEvent): void => {
				// Error info is available in onclose not in onerror
				const verb = this.reconnecting ? "Reconnect" : "Connect";
				this.log(ELogSeverity.error, `${verb} failed`, "createStateFullConnection", this, {
					target: this.target,
					closecode: closed.code,
				});

				socket.onSocketConnected = undefined;
				socket.onSocketClose = undefined;

				// Okay we have an error, let's notify the pending sockets about this error
				const error = new ENetUC_Common.AsnRequestError({
					iErrorDetail: CustomInvokeProblemEnum.serviceUnavailable,
					u8sErrorString: `${verb} to ${this.target} failed. WebSocket error ${closed.code}`,
				});
				this.pendingSockets.reject(error);

				if (this.autoReconnect) {
					// The client wants auto reconnect, so let's start the timer
					let timeout = 1000;
					this.reconnectCounter++;
					if (this.reconnectCounter >= 10)
						timeout = 5000;
					if (this.reconnecting) {
						this.log(ELogSeverity.warn, "reconnect failed", "clientReconnect", this, {
							reconnectCounter: this.reconnectCounter,
							timeout,
						}, error);
					}
					this.setReconnectTimeout(timeout);
				}

				this.openingStateFullConnection = false;
				resolve(false);
			};
		});
	}

	/**
	 * Helper function to create a websocket connection object
	 *
	 * @param caller - The caller of the shutdown (for logging and diagnostic)
	 */
	private shutdown(caller: string): void {
		this.setReconnectTimeout(0);
		if (this.socket) {
			this.socket.close();
			this.socket = undefined;
		}
		const err = new ENetUC_Common.AsnRequestError({
			iErrorDetail: CustomInvokeProblemEnum.serviceUnavailable,
			u8sErrorString: "TSASN1Client exit was called",
		});
		this.pendingSockets.reject(err);
	}

	/**
	 * Handler that is called if the client connection was closed
	 * This method is *NOT* ment to be called by subclasses. The subclass provides a socket notify object which then binds to these methods
	 *
	 * @param event - the websocket close event
	 */
	private onSocketClose(event: ISocketCloseEvent): void {
		this.log(ELogSeverity.error, "WebSocket was closed. Going to reconnect", "onSocketClose", this, {
			code: event.code,
			reason: event.reason,
		});
		this.fire_OnDisconnected(false).catch((error: unknown): void => {
			this.log(ELogSeverity.error, "fire_OnDisconnected exception catched", "onSocketClose", this, undefined, error);
		});
		this.shutdown("TSASN1Client.clientClose");
		this.setReconnectTimeout(1000);
	}

	/**
	 * Handler that is called if the client connection signalled an error
	 * This method is *NOT* ment to be called by subclasses. The subclass provides a socket notify object which then binds to these methods
	 *
	 * @param event - the websocket error event
	 */
	private onSocketError(event: ISocketErrorEvent): void {
		const readystate = this.socket ? TSASN1Client.getWebSocketReadyStateAsString(this.socket.readyState) : "undefined";
		this.log(ELogSeverity.error, "Websocket is in an error state", "onSocketError", this, { readystate });
		// we do not terminate the connection here as the onclose event handler will be called next (otherwise we remove the callback in exit and thus do not get called)
		// this.shutdown("TSASN1Client.clientError");
	}

	/**
	 * Handler that is called if the client connection received a full (non fragmented) message
	 * This handler is called with full messages (websocket, rest, for tcp the NodeClient takes care about the framing)
	 * This method is *NOT* ment to be called by subclasses. The subclass provides a socket notify object which then binds to these methods
	 *
	 * @param event - the websocket message event
	 */
	private onSocketMessage(event: ISocketMessageEvent): void {
		// Fill the invokecontext
		const invokeContext = new ReceiveInvokeContext({ clientConnectionID: this.clientConnectionID });
		// Call the receive method
		this.receive(event.data, invokeContext).then((response: IResponseData | undefined): void => {
			// If the receive returned data send it back
			if (response && this.socket && this.socket.readyState === ESocketState.OPEN)
				this.socket.send(response.payLoad);
		}).catch((error: unknown): void => {
			this.log(ELogSeverity.error, "exception", "onSocketMessage", this, {
				clientConnectionID: this.clientConnectionID,
			}, error);
		});
	}

	/**
	 * Helper that tries to reconnect to the target. Is called from outside to reestablish a connection in case the embedded reconnect mechanism isn't used.
	 *
	 * @returns - the current connection target
	 */
	public async clientReconnect(): Promise<boolean> {
		if (this.socket || this.openingStateFullConnection)
			return true;
		try {
			this.reconnecting = true;
			if (await this.createStateFullConnection())
				this.log(ELogSeverity.info, "successfully reconnected", "clientReconnect", this);
			return true;
		}
		catch (error: unknown) {
			return false;
		}
	}

	/**
	 * Notifies the connection callbacks about an established connection
	 *
	 * @param bReconnected - in case of a reconnect this property is set to true
	 */
	private async fire_OnConnected(bReconnected: boolean): Promise<void> {
		this.log(ELogSeverity.debug, "calling notifies", "fire_OnConnected", this, { bReconnected });
		try {
			for (const callback of this.connectionCallBack)
				await callback.onClientConnected(bReconnected);
		}
		catch (error) {
			this.log(ELogSeverity.error, "fire_OnConnected threw an exception", "fire_OnConnected", this, undefined, error);
		}
	}

	/**
	 * Notifies the connection callbacks about an disconnected connection
	 *
	 * @param bControlledDisconnect - in case of a controlled disconnect (e.g. shutdown or manually called disconnect) set to true
	 */
	private async fire_OnDisconnected(bControlledDisconnect: boolean): Promise<void> {
		this.log(ELogSeverity.debug, "calling notifies", "fire_OnDisconnected", this, { bControlledDisconnect });
		for (const callback of this.connectionCallBack)
			await callback.onClientDisconnected(bControlledDisconnect);
	}

	/**
	 * Notifies the connection callbacks about a connection attempt taking place
	 * The notified can then do things in advance (reset things, show a notification, fetch the proper server)
	 * Returning false will terminate the connect attempt and also terminate pending invokes which have been queued
	 *
	 * @param bReconnecting - true in case this is a reconnect attempt from the lib
	 */
	private async fire_OnBeforeConnect(bReconnecting: boolean): Promise<boolean> {
		this.log(ELogSeverity.debug, "calling notifies", "fire_OnBeforeConnect", this, { bReconnecting });

		let bContinue = true;
		for (const callback of this.connectionCallBack) {
			if (callback.onBeforeConnect) {
				if (!await callback.onBeforeConnect(bReconnecting)) {
					bContinue = false;
					break;
				}
			}
		}

		if (!bContinue)
			this.log(ELogSeverity.warn, "notified returned false, stopping connect", "fire_OnBeforeConnect", this);

		return bContinue;
	}
}
