// Centralised code for the TypeScript converters.
// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do not directly edit or modify the code as it is machine generated and will be overwritten with every compilation

// dprint-ignore-file
/* eslint-disable */

import { ASN1ClassInstanceType } from "./TSASN1Base.js";
import { IWebSocketOptions, TSASN1Client } from "./TSASN1Client.js";
import { EASN1TransportEncoding } from "./TSInvokeContext.js";
import { ELogSeverity, IConnectionSocket, ISocketCloseEvent, ISocketConnectedEvent, ISocketErrorEvent, ISocketMessageEvent } from "./TSROSEBase";
/**
 * The ASN1 client side as required in the browser (different websocket and timer)
 */
export class TSASN1BrowserClient extends TSASN1Client {
	// Our Browser websocket object
	private ws?: WebSocket;
	// The reconnecting timer
	private reconnectTimeout?: number;

	/**
	 * Constructs the Client
	 *
	 * @param encoding - sets the encoding for outbound calls
	 */
	public constructor(encoding: EASN1TransportEncoding.JSON | EASN1TransportEncoding.BER) {
		super(encoding, ASN1ClassInstanceType.TSASN1BrowserClient);
		this.onclose = this.onclose.bind(this);
		this.onerror = this.onerror.bind(this);
		this.onmessage = this.onmessage.bind(this);
		this.onconnected = this.onconnected.bind(this);
	}

	/**
	 * Creates a browser based websocket connection object and maps it into the IDualWebSocket interface
	 *
	 * @param address - The address to connect to
	 * @param options - Options as provided from the TSASN1Client
	 * @returns - A IDualWebSocket interface to map browser based websocket things into the IDualWebSocket interface or undefined if no websocket object was created
	 */
	protected getConnectionSocket(address: string, options?: IWebSocketOptions): IConnectionSocket | undefined {
		if (typeof WebSocket !== "undefined")
			this.ws = new WebSocket(address);
		else if (typeof self !== "undefined")
			this.ws = new self.WebSocket(address);

		if (!this.ws) {
			this.log(ELogSeverity.error, "Failed to open websocket", "getConnectionSocket", this, {
				mode: this.connectionMode,
			});
			return undefined;
		}
		
		// Simpler reading the data from the message event.
		this.ws.binaryType = "arraybuffer";

		this.ws.onclose = (event: CloseEvent) => this.onclose(event, socket);
		this.ws.onerror = (event: Event) => this.onerror(event, socket);
		this.ws.onmessage = (event: MessageEvent<string>) => this.onmessage(event, socket);
		this.ws.onopen = (event: Event) => this.onconnected(event, socket);
		const socket: IConnectionSocket = { readyState: this.ws.readyState, send: this.send, close: this.close };
		return socket;
	}

	/**
	 * WebSocket onclose handler, handed over to the idual.onclose
	 *
	 * @param event - Broser based CloseEvent
	 */
	private onclose(event: CloseEvent, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		if (!socket.onSocketClose)
			return;
		const close: ISocketCloseEvent = {
			code: event.code,
			reason: event.reason,
			wasClean: event.wasClean,
			source: event.target,
		};
		socket.onSocketClose(close);
	}

	/**
	 * WebSocket onerror handler, handed over to the idual.onerror
	 *
	 * @param event - Broser based Event
	 */
	private onerror(event: Event, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		if (!socket.onSocketError)
			return;
		const error: ISocketErrorEvent = {
			source: event.target,
			type: event.type,
		};
		socket.onSocketError(error);
	}

	/**
	 * WebSocket onmessage handler, handed over to the idual.onmessage
	 *
	 * @param event - Broser based MessageEvent
	 */
	private onmessage(event: MessageEvent<string>, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		if (!socket.onSocketMessage)
			return;
		const data = this.prepareData(event.data);
		if (data) {
			const msg: ISocketMessageEvent = { data, type: event.type, source: event.target };
			socket.onSocketMessage(msg);
		}
	}

	/**
	 * WebSocket onopen handler, handed over to the idual.onopen
	 *
	 * @param event - Broser based Event
	 */
	private onconnected(event: Event, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		if (!socket.onSocketConnected)
			return;
		const open: ISocketConnectedEvent = { source: event.target, type: event.type };
		socket.onSocketConnected(open);
	}

	/**
	 * Called to close the websocket. Calls the browser based websocket close function
	 *
	 * @param code - Close code
	 * @param reason - Close reason
	 */
	private close(code?: number, reason?: string): void {
		if (this.ws)
			this.ws.close(code, reason);
	}

	/**
	 * Called to send data through the websocket. Calls the browser based websocket send function
	 *
	 * @param data - data to send through the websocket
	 */
	private send(data: string | Uint8Array): void {
		if (this.ws)
			this.ws.send(data);
	}

	/**
	 * Wrapper for the browser based fetch method
	 *
	 * @param input - The url we want to fetch data from
	 * @param init - A RequestInit object
	 * @returns - A Promise resolving into a Response object
	 */
	protected async fetch(input: string, init?: RequestInit): Promise<Response> {
		return fetch(input, init);
	}

	/**
	 * Set a node based timer for the clientReconnect method with duration timeout
	 *
	 * @param timeout - The timeout after which clientReconnect is called
	 */
	protected setReconnectTimeout(timeout: number): void {
		if (this.reconnectTimeout) {
			clearTimeout(this.reconnectTimeout);
			this.reconnectTimeout = undefined;
		}
		if (timeout && this.autoreconnect)
			this.reconnectTimeout = self.setTimeout(this.clientReconnect, timeout);
	}

	/**
	 * Prepares data from the message event to the notastion we need (Differs between node and browser implementation)
	 *
	 * @param event - the message that contains the payload
	 * @returns data on successs or undefined on error
	 */
	protected prepareData(data: string | ArrayBuffer): Uint8Array | object | undefined {
		if (typeof data === "string")
			return JSON.parse(data) as object;
		if (data instanceof ArrayBuffer)
			return new Uint8Array(data);

		this.log(ELogSeverity.error, "exception", "Received unhandled data", this);
		debugger;
		return undefined;
	}

	/**
	 * Updates the idual socket
	 *
	 * @param socket - the shared socket object which is handed over to the base class
	 */
	protected updateSocketState(socket: IConnectionSocket): void {
		if (this.ws)
			socket.readyState = this.ws.readyState;
	}	
}
