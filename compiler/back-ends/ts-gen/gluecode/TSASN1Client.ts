// Centralised code for the TypeScript converters.
// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do not directly edit or modify the code as it is machine generated and will be overwritten with every compilation

// prettier-ignore
/* eslint-disable */

import { ROSEError, ROSEReject, ROSEResult } from "./SNACCROSE";
import { ASN1ClassInstanceType, PendingInvoke, TSASN1Base } from "./TSASN1Base";
import { EHttpHeaders, ELogSeverity, IASN1Transport, IDualWebSocket, EDualWebSocketState, IDualWebSocketCloseEvent, ISendInvokeContext, ReceiveInvokeContext, CustomInvokeProblemEnum, createInvokeReject, IASN1InvokeData, ROSEBase, IDualWebSocketMessageEvent } from "./TSROSEBase";
import { EASN1TransportEncoding } from "./TSInvokeContext";
import * as ENetUC_Common from "./ENetUC_Common";

export interface IDualWebSocketOptions {
	perMessageDeflate?: boolean;
	headers?: { [key: string]: string };
}

// Node is not aware of this type but node-fetch which we use until node itself supports fetch uses it that way
type HeadersInit = string[][] | Record<string, string> | Headers;

/**
 * A Promise that is fullfilled if the requested websocket was created or rejected if the creation failed
 */
class WebSocketPromise {
	public readonly resolve: (value: ENetUC_Common.AsnRequestError | IDualWebSocket) => void;
	public readonly reject: (reason?: unknown) => void;
	/**
	 * Constructs a websocket promise object, simply stores the handed over arguments in the class
	 *
	 * @param resolve - The resolve method that is called if the websocket was created
	 * @param reject - The rejcet method that is called if something unhandled did occur
	 */
	public constructor(resolve: (value: ENetUC_Common.AsnRequestError | IDualWebSocket) => void, reject: (reason?: unknown) => void) {
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
	UNKNOWN = 0,
	WEBSOCKET = 1,
	REST = 2,
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
	protected connectionMode: EASNCONNECTIONMODE = EASNCONNECTIONMODE.UNKNOWN;

	// The client Websocket towards the server
	// Only beeing used if the connection is using websockets (target points to ws or wss)
	protected ws?: IDualWebSocket = undefined;

	// These Properties are ONLY used if the client is using websockets to connect to the server:
	// -------------------------------------
	// The client automatically tries to reconnect the target if the connection was dropped
	protected autoreconnect = true;
	// The sessionID the server provided for this client (has to be filled by the businesslogik as soon as a session id is available)
	protected clientConnectionID?: string;
	// List of pending requests for a websocket towards the target
	// As long as the connection is beeing established every request for a websocket is qued
	// This list is automatically cleared if the connection failed.
	protected pendingwebsockets: WebSocketPromise[] = [];

	// We are currently reconnecting
	private basn1ClientReconnecting = false;
	// Helper to parametrise the reconnect timer in case of connection failures (first 10 approaces retry every second, afterwards every 5 seconds)
	private asn1ClientReconnectCounter = 0;
	// We are currently opening a websocket (asynchronous function)
	private basn1ClientOpeningWebSocket = false;
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
	protected constructor(encoding: EASN1TransportEncoding.JSON | EASN1TransportEncoding.BER, instanceType: ASN1ClassInstanceType) {
		super(encoding, instanceType);
		this.onClientClose = this.onClientClose.bind(this);
		this.onClientMessage = this.onClientMessage.bind(this);
		this.onClientError = this.onClientError.bind(this);
	}

	/**
	 * Helper method that provides the websocket state as text for logging and debugging
	 *
	 * @param state - The state the method should provide as string
	 * @returns - the state as text or UNKNOWN if an unknown state was provided
	 */
	public static getWebSocketReadyStateAsString(state: number): string {
		switch (state) {
			case 0:
				return "CONNECTING";
			case 1:
				return "OPEN";
			case 2:
				return "CLOSING";
			case 3:
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
	public getWebSocketState(): EDualWebSocketState | undefined {
		if (!this.ws)
			return undefined;
		return this.ws.readyState;
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
		if (this.ws && this.ws.readyState === EDualWebSocketState.OPEN) {
			const encoded = ROSEBase.encodeToTransport(data.payLoad, this.encodeContext);
			this.ws.send(encoded.payLoad);
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
			let target = this.target;

			// Allows to specify a REST target through the invokeContext for this request (indipendently from the this.target)
			if (data.invokeContext?.restTarget) {
				connectionMode = EASNCONNECTIONMODE.REST;
				target = data.invokeContext?.restTarget;
			}

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
					if (timeout)
						timerID = setTimeout((): void => { this.onROSETimeout(id); }, timeout);
					const pending = new PendingInvoke(data.invoke, resolve, reject, timerID);
					this.pendingInvokes.set(id, pending);
					resolveUndefined = false;
				}
			}

			if (connectionMode === EASNCONNECTIONMODE.WEBSOCKET) {
				// Get or create a connection to the target
				this.getConnection(data.invokeContext).then((ws: IDualWebSocket | ENetUC_Common.AsnRequestError): void => {
					if (!ws || ws instanceof ENetUC_Common.AsnRequestError) {
						// Could not connect to the target or an unknown error occured
						let invokeReject: ROSEReject;
						if (ws instanceof ENetUC_Common.AsnRequestError)
							invokeReject = createInvokeReject(data.invoke, ws.iErrorDetail, ws.u8sErrorString);
						else
							invokeReject = createInvokeReject(data.invoke, CustomInvokeProblemEnum.serviceUnavailable, `Could not connect to ${target}`);
						// Handl rose reject if we did not successfully connect to the target
						this.log(ELogSeverity.error, "Could not establish connection", "sendInvoke", this, ws);
						this.onROSEReject(invokeReject, receiveInvokeContext);
						throw (invokeReject);
					} else {
						const encoded = ROSEBase.encodeToTransport(data.payLoad, this.encodeContext);
						this.logTransport(encoded.logData, "sendInvoke", "out", data.invokeContext);
						// Send the message
						ws.send(encoded.payLoad);
					}
				}).catch((error): void => {
					this.log(ELogSeverity.error, "exception", "sendInvoke", this, undefined, error);
					this.handlePendingWebsockets(error);
					reject(error);
				});
			} else if (connectionMode === EASNCONNECTIONMODE.REST) {
				const encoding = data.invokeContext.encoding;
				const headers: HeadersInit = {
					"Content-Type": encoding === EASN1TransportEncoding.JSON ? "application/json" : "application/octet-stream"
				};

				// Add the method we are calling to the url
				if (target.charAt(target.length - 1) !== "/")
					target += "/";
				target += data.invokeContext.operationName;
				
				const encoded = ROSEBase.encodeToTransport(data.payLoad, this.encodeContext);
				// Contains the encoded data for the transport
				const body = encoded.payLoad;
				// Contains the encoded data for logging
				const logData = encoded.logData;

				// Set the additional headers from the class object
				if (this.additionalHeaders) {
					const keys = Object.keys(this.additionalHeaders);
					for (const key of keys) {
						const obj = this.additionalHeaders[key];
						if (obj && typeof (obj) === "string")
							headers[key] = obj;
					}
				}

				// Set the additional headers from the invoke object
				if (data.invokeContext?.headers) {
					const keys = Object.keys(data.invokeContext.headers);
					for (const key of keys) {
						const obj = data.invokeContext.headers[key];
						if (obj && typeof (obj) === "string")
							headers[key] = obj;
					}
				}

				// We can either send the request with ROSE envelop or without (not really needed here), default is without

				// Build the http request object
				const requestdata = {
					method: "POST",
					body,
					headers
				};

				// REST requests are handled through the pending invokes list
				// Every request is added to this list no matter if we defined a timeout or not
				// So we do not directly see the handling of the result.
				// The result is handled through the regular methods and completed in the background
				this.logTransport(encoded.logData, "sendInvoke", "out", data.invokeContext);
				this.fetch(target, requestdata).then(async (response: Response): Promise<boolean> => {
					try {
						// We received a result (Will this work for BER encoding as well?)
						let message: Uint8Array | object;
						if (encoding === EASN1TransportEncoding.BER) {
							const buffer = await response.arrayBuffer();
							message = new Uint8Array(buffer);
						} else
							message = (await response.json()) as object;
						await this.receive(message, receiveInvokeContext);
					} catch (error) {
						// We received something else -> create reject object and process it
						const reject = createInvokeReject(data.invoke, CustomInvokeProblemEnum.missingResponse, "Exception while handling server response");
						this.onROSEReject(reject, receiveInvokeContext);
					}
					return false; // We are not really interested in this result or handle it
				}).catch((error) => {
					this.log(ELogSeverity.error, "Could not establish connection", "sendInvoke", this, undefined, error);
					const reject = createInvokeReject(data.invoke, CustomInvokeProblemEnum.serviceUnavailable, `Could not connect to ${target}`);
					this.onROSEReject(reject, receiveInvokeContext);
					return false; // We are not really interested in this result or handle it
				});
			} else {
				if (target === "")
					throw new Error(`You need to specify a connection target either through setTarget() or the ISendInvokeContext`);
				else
					throw new Error(`Unsupported target ${target}`);
			}
			if (resolveUndefined)
				resolve(undefined);
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
		if (this.connectionMode === EASNCONNECTIONMODE.WEBSOCKET) {
			try {
				const result = await this.getConnection(invokeContext);
				if (!result || result instanceof ENetUC_Common.AsnRequestError) {
					this.log(ELogSeverity.error, "Could not establish connection", "connect", this, result);
					return false;
				} else
					return true;
			} catch (error) {
				this.log(ELogSeverity.error, "exception", "connect", this, undefined, error);
				return false;
			}
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
	protected setTarget(newTarget: string): void {
		const oldTarget = this.target;
		try {
			if (this.target !== newTarget) {
				this.log(ELogSeverity.debug, "Setting new Target", "setTarget", this, { oldTarget, newTarget });
				this.target = newTarget;
				const targetlc = this.target.toLowerCase();
				if (targetlc.substring(0, 2) === "ws")
					this.connectionMode = EASNCONNECTIONMODE.WEBSOCKET;
				else if (targetlc.substring(0, 4) === "http")
					this.connectionMode = EASNCONNECTIONMODE.REST;
				else {
					this.connectionMode = EASNCONNECTIONMODE.UNKNOWN;
					if (newTarget.length)
						throw new Error(`Unknown connection target protocol ${this.target}, expecting websocket or http rest like (ws,wss,http,https)`);
				}
				if (this.ws) {
					this.disconnect(true).then(() => { }).catch((error) => {
						this.log(ELogSeverity.error, "Failed to disconnect websocket", "setTarget", this, { oldTarget, newTarget }, error);
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
	protected abstract getWebSocket(address: string, options?: IDualWebSocketOptions): IDualWebSocket | undefined;
	protected abstract fetch(input: string, init?: RequestInit): Promise<Response>;
	protected abstract asn1ClientsetReconnectTimeout(timeout: number): void;
	protected abstract prepareData(event: IDualWebSocketMessageEvent): Promise<Uint8Array | object | undefined>;

	/**
	 * Helper function to get or create a websocket connection object
	 *
	 * @param invokeContext - contextual data provided with the invoke
	 * @returns - An IDualWebSocket object on success or an AsnRequestError on error
	 */
	private async getConnection(invokeContext?: ISendInvokeContext): Promise<IDualWebSocket | ENetUC_Common.AsnRequestError> {
		return new Promise((resolve, reject): void => {
			if (this.ws && this.ws.readyState === EDualWebSocketState.OPEN) {
				// If we already have a usable websocket object return it
				resolve(this.ws);
			} else if (!this.basn1ClientOpeningWebSocket) {
				// If we do not yet have one set that we are creating one
				this.basn1ClientOpeningWebSocket = true;
				// Add our request to the front of the pending websocket objects
				// The list is resolved in createWebSocketConnection on success or here in case of an error
				this.pendingwebsockets.unshift(new WebSocketPromise(resolve, reject));
				this.createWebSocketConnection(invokeContext).then((): void => {
					this.basn1ClientOpeningWebSocket = false;
				}).catch((error): void => {
					// If we did not get one add our promise as FIRST object in the pening list
					// The pending list wil be handled in the handlePendingWebsockets
					this.handlePendingWebsockets(error);
					this.basn1ClientOpeningWebSocket = false;
				});
			} else {
				// We are currently creating a websocket object, add our request to the pending list
				this.pendingwebsockets.push(new WebSocketPromise(resolve, reject));
			}
		});
	}

	/**
	 * Helper function to create a websocket connection object
	 *
	 * @param invokeContext - contextual data provided with the invoke
	 * @returns - resolved true or reject with an AsnRequestError
	 */
	private async createWebSocketConnection(invokeContext?: ISendInvokeContext): Promise<true> {
		if (this.ws)
			return true;

		return new Promise((resolve, reject): void => {
			// Call the node browser implementation to create a websocket object

			const options: IDualWebSocketOptions = {
				perMessageDeflate: false
			};

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

			const con = this.getWebSocket(this.target, options);
			if (!con)
				throw new Error("Failed to get a websocket object");
			/** called if the websocket was opend (connect to the target */
			con.onopen = (): void => {
				// In case we are connected
				con.onopen = null;
				con.onclose = null;
				this.log(ELogSeverity.info, "Connected to", "createWebSocketConnection", this, { target: this.target });

				// Add event listener
				con.addEventListener("close", this.onClientClose);
				con.addEventListener("message", this.onClientMessage);
				con.addEventListener("error", this.onClientError);
				this.ws = con;

				// tell the notifies that we are connected
				this.fire_OnConnected(this.basn1ClientReconnecting).then(() => {
					this.basn1ClientReconnecting = false;

					// Init the reconnect timeout
					this.asn1ClientsetReconnectTimeout(5000);

					// Handle all pending operations
					for (const pendingcallback of this.pendingwebsockets)
						pendingcallback.resolve(con);
					this.pendingwebsockets = [];

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
			con.onclose = (closed: IDualWebSocketCloseEvent): void => {
				// Error info is available in onclose not in onerror
				con.onopen = null;
				con.onclose = null;
				this.log(ELogSeverity.error, "connecting failed", "createWebSocketConnection", this, { target: this.target, closecode: closed.code });

				// Init the reconnect after 1 second
				this.asn1ClientsetReconnectTimeout(1000);

				// reject with error
				reject(new ENetUC_Common.AsnRequestError({
					iErrorDetail: CustomInvokeProblemEnum.serviceUnavailable,
					u8sErrorString: `Connect to ${this.target} failed. WebSocket error ${closed.code}`
				}));
			};
		});
	}

	/**
	 * Helper function to create a websocket connection object
	 *
	 * @param caller - The caller of the shutdown (for logging and diagnostic)
	 */
	private shutdown(caller: string): void {
		this.asn1ClientsetReconnectTimeout(0);
		if (this.ws) {
			this.ws.removeEventListener("close", this.onClientClose);
			this.ws.removeEventListener("message", this.onClientMessage);
			this.ws.removeEventListener("error", this.onClientError);
			this.ws.close();
			this.ws = undefined;
		}
		const err = new ENetUC_Common.AsnRequestError({
			iErrorDetail: CustomInvokeProblemEnum.serviceUnavailable,
			u8sErrorString: "TSASN1Client exit was called"
		});
		this.handlePendingWebsockets(err);
	}

	/**
	 * Handler that is called if the client connection was closed
	 *
	 * @param event - the websocket close event
	 */
	private async onClientClose(event: IDualWebSocketCloseEvent): Promise<void> {
		this.log(ELogSeverity.error, "WebSocket was closed. Going to reconnect", "onClientClose", this, { code: event.code, reason: event.reason });
		await this.fire_OnDisconnected(false);

		this.shutdown("TSASN1Client.clientClose");
		this.asn1ClientsetReconnectTimeout(1000);
	}

	/**
	 * Handler that is called if the client connection signalled an error
	 *
	 * @param event - the websocket error event
	 */
	private onClientError(event: Event): void {
		const readystate = this.ws ? TSASN1Client.getWebSocketReadyStateAsString(this.ws.readyState) : "undefined";
		this.log(ELogSeverity.error, "Websocket is in an error state", "onClientError", this, { readystate });
		// we do not terminate the connection here as the onclose event handler will be called next (otherwise we remove the callback in exit and thus do not get called)
		// this.shutdown("TSASN1Client.clientError");
	}

	/**
	 * Handler that is called if the client connection received websocket message
	 *
	 * @param event - the websocket message event
	 */
	private async onClientMessage(event: IDualWebSocketMessageEvent): Promise<void> {
		if (event.data) {
			// Fill the invokecontext
			const invokeContext = new ReceiveInvokeContext({
				clientConnectionID: this.clientConnectionID
			});
			try {
				const rawData = await this.prepareData(event);
				if (rawData) {
					// Call the receive method
					const response = await this.receive(rawData, invokeContext);
					// If the receive returned data send it back
					if (response && this.ws && this.ws.readyState === EDualWebSocketState.OPEN)
						this.ws.send(response.payLoad);
				}
			} catch (error) {
				this.log(ELogSeverity.error, "exception", "onClientMessage", this, undefined, error);
			}
		}
	}

	/**
	 * Helper that tries to reconnect to the target. Is called from outside to reestablish a connection in case the embedded reconnect mechanism isn´t used.
	 *
	 * @returns - the current connection target
	 */
	public async asn1ClientReconnect(): Promise<boolean> {
		if (this.ws || this.basn1ClientOpeningWebSocket)
			return true;
		try {
			this.basn1ClientReconnecting = true;
			await this.fire_OnBeforeReconnect();
			await this.createWebSocketConnection();
			this.log(ELogSeverity.info, "successfully reconnected", "asn1ClientReconnect", this);
			return true;
		} catch (error) {
			this.handlePendingWebsockets(error);
			let timeout = 1000;
			this.asn1ClientReconnectCounter++;
			if (this.asn1ClientReconnectCounter >= 10)
				timeout = 5000;
			this.log(ELogSeverity.warn, "reconnect failed", "asn1ClientReconnect", this, { reconnectCounter: this.asn1ClientReconnectCounter, timeout }, error);
			this.asn1ClientsetReconnectTimeout(timeout);
			return false;
		}
	}

	/**
	 * In case of a websocket connecting error we fullfill all pending callbacks with the error object
	 *
	 * @param err - The error object as provided by the caller
	 */
	private handlePendingWebsockets(err: unknown): void {
		if (!(err instanceof ENetUC_Common.AsnRequestError)) {
			err = new ENetUC_Common.AsnRequestError({
				iErrorDetail: CustomInvokeProblemEnum.serviceUnavailable,
				u8sErrorString: (err as Error).message
			});
		}
		for (const pendingcallback of this.pendingwebsockets)
			pendingcallback.reject(err);
		this.pendingwebsockets = [];
	}

	/**
	 * Notifies the connection callbacks about an established connection
	 *
	 * @param bReconnected - in case of a reconnect this property is set to true
	 */
	private async fire_OnConnected(bReconnected: boolean): Promise<void> {
		for (const callback of this.connectionCallBack)
			await callback.onClientConnected(this.basn1ClientReconnecting);
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
