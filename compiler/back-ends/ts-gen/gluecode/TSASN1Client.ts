// Centralised code for the TypeScript converters.
// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do not directly edit or modify the code as it is machine generated and will be overwritten with every compilation

// dprint-ignore-file
/* eslint-disable */

import * as ENetUC_Common from "./ENetUC_Common";
import { ROSEError, ROSEReject, ROSEResult } from "./SNACCROSE";
import { ASN1ClassInstanceType, PendingInvoke, TSASN1Base } from "./TSASN1Base";
import { EASN1TransportEncoding } from "./TSInvokeContext";
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
} from "./TSROSEBase";

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
	public readonly resolve: (value: ENetUC_Common.AsnRequestError | IConnectionSocket) => void;
	public readonly reject: (reason?: unknown) => void;
	/**
	 * Constructs a websocket promise object, simply stores the handed over arguments in the class
	 *
	 * @param resolve - The resolve method that is called if the websocket was created
	 * @param reject - The rejcet method that is called if something unhandled did occur
	 */
	public constructor(
		resolve: (value: ENetUC_Common.AsnRequestError | IConnectionSocket) => void,
		reject: (reason?: unknown) => void,
	) {
		this.resolve = resolve;
		this.reject = reject;
	}
}

/**
 * Connection related callbacks if we get connected or disconnected
 */
export interface IClientConnectionCallback {
	// Is called before the reconnect is done (allows the client to do necessary things in advance (calculcate tokens, fetch the proper target url))
	onBeforeReconnect?(): Promise<void>;
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
	protected autoreconnect = true;
	// The sessionID the server provided for this client (has to be filled by the businesslogik as soon as a session id is available)
	protected clientConnectionID?: string;
	// List of pending requests for a websocket towards the target
	// As long as the connection is beeing established every request for a websocket is qued
	// This list is automatically cleared if the connection failed.
	protected pendingSockets: ConnectionSocketPromise[] = [];

	// We are currently reconnecting
	private bClientReconnecting = false;
	// Helper to parametrise the reconnect timer in case of connection failures (first 10 approaces retry every second, afterwards every 5 seconds)
	private nClientReconnectCounter = 0;
	// We are currently opening a statefull connection (asynchronous function)
	private bClientOpeningStateFullConnection = false;
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
		this.onClientClose = this.onClientClose.bind(this);
		this.onClientMessage = this.onClientMessage.bind(this);
		this.onClientError = this.onClientError.bind(this);
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
		const receiveInvokeContext = ReceiveInvokeContext.create(data.invoke);

		return new Promise((resolve, reject): void => {
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
					const pending = new PendingInvoke(data.invoke, resolve, reject, timerID);
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
	 * @param reject - the reject method of the sendInvoke promise to reject in case of a connection error
	 */
	private sendStatefull(data: IASN1InvokeData): void {
		// Get or create a connection to the target
		this.getConnection(data.invokeContext).then((socket: IConnectionSocket | ENetUC_Common.AsnRequestError): void => {
			if (!socket || socket instanceof ENetUC_Common.AsnRequestError) {
				// Could not connect to the target or an unknown error occured
				let invokeReject: ROSEReject;
				if (socket instanceof ENetUC_Common.AsnRequestError)
					invokeReject = createInvokeReject(data.invoke, socket.iErrorDetail, socket.u8sErrorString);
				else {
					invokeReject = createInvokeReject(
						data.invoke,
						CustomInvokeProblemEnum.serviceUnavailable,
						`Could not connect to ${this.target}`,
					);
				}
				// Handl rose reject if we did not successfully connect to the target
				this.log(ELogSeverity.error, "Could not establish connection", "sendWebSocket", this, socket);
				const receiveInvokeContext = ReceiveInvokeContext.create(data.invoke);
				this.onROSEReject(invokeReject, receiveInvokeContext);
				throw invokeReject;
			} else {
				const encodeResult = ROSEBase.encodeToTransport(data.payLoad, this.encodeContext);
				this.logTransport(encodeResult.logData, "sendWebSocket", "out", data.invokeContext);
				// Send the message
				socket.send(encodeResult.payLoad);
			}
		}).catch((error): void => {
			this.log(ELogSeverity.error, "exception", "sendWebSocket", this, undefined, error);
			this.handlePendingSockets(error);
			throw error;
		});
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
		const encoding = data.invokeContext.encoding;
		const headers: HeadersInit = {
			"Content-Type": encoding === EASN1TransportEncoding.JSON ? "application/json" : "application/octet-stream",
		};

		// Allows to specify a REST target through the invokeContext for this request (indipendently from the this.target)
		let target = data.invokeContext?.restTarget || this.target;

		// Add the method we are calling to the url
		if (target.charAt(target.length - 1) !== "/")
			target += "/";
		target += data.invokeContext.operationName;

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
			headers: { "Content-Type": "application/json" },
		};

		// REST requests are handled through the pending invokes list
		// Every request is added to this list no matter if we defined a timeout or not
		// So we do not directly see the handling of the result.
		// The result is handled through the regular methods and completed in the background
		this.logTransport(encodeResult.logData, "sendInvoke", "out", data.invokeContext);
		const receiveInvokeContext = ReceiveInvokeContext.create(data.invoke);
		this.fetch(target, requestdata).then(async (response: Response): Promise<boolean> => {
			if (response.status < 200 || response.status > 299) {
				const reject = createInvokeReject(data.invoke, response.status, response.statusText);
				this.onROSEReject(reject, receiveInvokeContext);
			} else {
				try {
					// We received a result (Will this work for BER encoding as well?)
					let message: Uint8Array | object;
					if (encoding === EASN1TransportEncoding.BER) {
						const buffer = await response.arrayBuffer();
						message = new Uint8Array(buffer);
					} else {
						message = (await response.json()) as object;
					}
					await this.receive(message, receiveInvokeContext);
				} catch (error) {
					this.log(ELogSeverity.error, "receiving failed with exception", "sendInvoke", this, undefined, error);
					// We received something else -> create reject object and process it
					const reject = createInvokeReject(
						data.invoke,
						CustomInvokeProblemEnum.missingResponse,
						"Exception while handling server response",
					);
					this.onROSEReject(reject, receiveInvokeContext);
				}
			}
			return false; // We are not really interested in this result or handle it
		}).catch((error) => {
			this.log(ELogSeverity.error, "Could not establish connection", "sendInvoke", this, undefined, error);
			const reject = createInvokeReject(
				data.invoke,
				CustomInvokeProblemEnum.serviceUnavailable,
				`Could not connect to ${target}`,
			);
			this.onROSEReject(reject, receiveInvokeContext);
			return false; // We are not really interested in this result or handle it
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
					const result = await this.getConnection(invokeContext);
					if (!result || result instanceof ENetUC_Common.AsnRequestError)
						this.log(ELogSeverity.error, "Could not establish connection", "connect", this, result);
					else
						return true;
				} catch (error) {
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
						this.log(
							ELogSeverity.error,
							"Failed to disconnect websocket",
							"setTarget",
							this,
							{ oldTarget, newTarget },
							error,
						);
					});
				}
			}
		} catch (error) {
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
	protected abstract prepareData(event: ISocketMessageEvent): Promise<Uint8Array | object | undefined>;

	/**
	 * Helper function to get or create a websocket connection object
	 *
	 * @param invokeContext - contextual data provided with the invoke
	 * @returns - An IConnectionSocket object on success or an AsnRequestError on error
	 */
	private async getConnection(
		invokeContext?: ISendInvokeContext,
	): Promise<IConnectionSocket | ENetUC_Common.AsnRequestError> {
		return new Promise((resolve, reject): void => {
			if (this.socket && this.socket.readyState === ESocketState.OPEN) {
				// If we already have a usable websocket object return it
				resolve(this.socket);
			} else if (!this.bClientOpeningStateFullConnection) {
				// If we do not yet have one set that we are creating one
				this.bClientOpeningStateFullConnection = true;
				// Add our request to the front of the pending websocket objects
				// The list is resolved in createWebSocketConnection on success or here in case of an error
				this.pendingSockets.unshift(new ConnectionSocketPromise(resolve, reject));
				this.createStateFullConnection(invokeContext).then((): void => {
					this.bClientOpeningStateFullConnection = false;
				}).catch((error): void => {
					// If we did not get one add our promise as FIRST object in the pening list
					// The pending list wil be handled in the handlePendingWebsockets
					this.handlePendingSockets(error);
					this.bClientOpeningStateFullConnection = false;
				});
			} else {
				// We are currently creating a websocket object, add our request to the pending list
				this.pendingSockets.push(new ConnectionSocketPromise(resolve, reject));
			}
		});
	}

	/**
	 * Helper function to create a statefull connection object
	 *
	 * @param invokeContext - contextual data provided with the invoke
	 * @returns - resolved true or reject with an AsnRequestError
	 */
	private async createStateFullConnection(invokeContext?: ISendInvokeContext): Promise<true> {
		if (this.socket)
			return true;

		return new Promise((resolve, reject): void => {
			// Call the node browser implementation to create a websocket object

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
			socket.onconnected = (): void => {
				// In case we are connected
				this.log(ELogSeverity.info, "Connected to", "createStateFullConnection", this, { target: this.target });

				// Add event listener
				this.socket = socket;

				// tell the notifies that we are connected
				this.fire_OnConnected(this.bClientReconnecting).then(() => {
					this.bClientReconnecting = false;

					// Init the reconnect timeout
					this.setReconnectTimeout(5000);

					// Handle all pending operations
					for (const pendingcallback of this.pendingSockets)
						pendingcallback.resolve(socket);
					this.pendingSockets = [];

					// resolve success
					resolve(true);
				}).catch((error) => {
					reject(error);
				});
			};
			/**
			 * called if the websocket was closed (opening failed)
			 *
			 * @param closed - the id of the closing
			 */
			socket.onclose = (closed: ISocketCloseEvent): void => {
				// Error info is available in onclose not in onerror
				this.log(ELogSeverity.error, "connecting failed", "createStateFullConnection", this, {
					target: this.target,
					closecode: closed.code,
				});

				// Init the reconnect after 1 second
				this.setReconnectTimeout(1000);

				// reject with error
				reject(
					new ENetUC_Common.AsnRequestError({
						iErrorDetail: CustomInvokeProblemEnum.serviceUnavailable,
						u8sErrorString: `Connect to ${this.target} failed. WebSocket error ${closed.code}`,
					}),
				);
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
		this.handlePendingSockets(err);
	}

	/**
	 * Handler that is called if the client connection was closed
	 *
	 * @param event - the websocket close event
	 */
	protected async onClientClose(event: ISocketCloseEvent): Promise<void> {
		this.log(ELogSeverity.error, "WebSocket was closed. Going to reconnect", "onClientClose", this, {
			code: event.code,
			reason: event.reason,
		});
		await this.fire_OnDisconnected(false);

		this.shutdown("TSASN1Client.clientClose");
		this.setReconnectTimeout(1000);
	}

	/**
	 * Handler that is called if the client connection signalled an error
	 *
	 * @param event - the websocket error event
	 */
	protected onClientError(event: ISocketErrorEvent): void {
		const readystate = this.socket ? TSASN1Client.getWebSocketReadyStateAsString(this.socket.readyState) : "undefined";
		this.log(ELogSeverity.error, "Websocket is in an error state", "onClientError", this, { readystate });
		// we do not terminate the connection here as the onclose event handler will be called next (otherwise we remove the callback in exit and thus do not get called)
		// this.shutdown("TSASN1Client.clientError");
	}

	/**
	 * Handler that is called if the client connection received websocket message
	 *
	 * @param event - the websocket message event
	 */
	protected async onClientMessage(event: ISocketMessageEvent): Promise<void> {
		if (event.data) {
			// Fill the invokecontext
			const invokeContext = new ReceiveInvokeContext({ clientConnectionID: this.clientConnectionID });
			try {
				// !!! We need to packetize here !!!
				// Welcher Transport, welches encoding
				const rawData = await this.prepareData(event);
				if (rawData) {
					// Call the receive method
					const response = await this.receive(rawData, invokeContext);
					// If the receive returned data send it back
					if (response && this.socket && this.socket.readyState === ESocketState.OPEN)
						this.socket.send(response.payLoad);
				}
			} catch (error) {
				this.log(ELogSeverity.error, "exception", "onClientMessage", this, undefined, error);
			}
		}
	}

	/**
	 * Handler that is called if the client connection has been opened
	 *
	 * @param event - the websocket message event
	 */
	protected async onClientConnected(event: ISocketConnectedEvent): Promise<void> {
	}

	/**
	 * Helper that tries to reconnect to the target. Is called from outside to reestablish a connection in case the embedded reconnect mechanism isn´t used.
	 *
	 * @returns - the current connection target
	 */
	public async clientReconnect(): Promise<boolean> {
		if (this.socket || this.bClientOpeningStateFullConnection)
			return true;
		try {
			this.bClientReconnecting = true;
			await this.fire_OnBeforeReconnect();
			await this.createStateFullConnection();
			this.log(ELogSeverity.info, "successfully reconnected", "asn1ClientReconnect", this);
			return true;
		} catch (error) {
			this.handlePendingSockets(error);
			let timeout = 1000;
			this.nClientReconnectCounter++;
			if (this.nClientReconnectCounter >= 10)
				timeout = 5000;
			this.log(ELogSeverity.warn, "reconnect failed", "asn1ClientReconnect", this, {
				reconnectCounter: this.nClientReconnectCounter,
				timeout,
			}, error);
			this.setReconnectTimeout(timeout);
			return false;
		}
	}

	/**
	 * In case of a statefull connection error we fullfill all pending sockets with the error object
	 *
	 * @param err - The error object as provided by the caller
	 */
	private handlePendingSockets(err: unknown): void {
		if (!(err instanceof ENetUC_Common.AsnRequestError)) {
			err = new ENetUC_Common.AsnRequestError({
				iErrorDetail: CustomInvokeProblemEnum.serviceUnavailable,
				u8sErrorString: (err as Error).message,
			});
		}
		for (const pendingSocket of this.pendingSockets)
			pendingSocket.reject(err);
		this.pendingSockets = [];
	}

	/**
	 * Notifies the connection callbacks about an established connection
	 *
	 * @param bReconnected - in case of a reconnect this property is set to true
	 */
	private async fire_OnConnected(bReconnected: boolean): Promise<void> {
		for (const callback of this.connectionCallBack)
			await callback.onClientConnected(this.bClientReconnecting);
	}

	/**
	 * Notifies the connection callbacks about an disconnected connection
	 *
	 * @param bControlledDisconnect - in case of a controlled disconnect (e.g. shutdown or manually called disconnect) set to true
	 */
	private async fire_OnDisconnected(bControlledDisconnect: boolean): Promise<void> {
		for (const callback of this.connectionCallBack)
			await callback.onClientDisconnected(bControlledDisconnect);
	}

	/**
	 * Notifies the connection callbacks about a reconnect taking place
	 * The notified can then do things in advance (reset things, show a notification, fetch the proper server)
	 */
	private async fire_OnBeforeReconnect(): Promise<void> {
		for (const callback of this.connectionCallBack) {
			if (callback.onBeforeReconnect)
				await callback.onBeforeReconnect();
		}
	}
}
