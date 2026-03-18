// Centralised code for the TypeScript converters.
// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do not directly edit or modify the code as it is machine generated and will be overwritten with every compilation

// dprint-ignore-file
/* eslint-disable */
// IF NODE<22
import fetch, { RequestInit as FetchInit } from "node-fetch";
// ENDIF
import net from "node:net";
import WebSocket from "ws";
import { ASN1ClassInstanceType } from "./TSASN1Base";
import { EASNCONNECTIONMODE, IWebSocketOptions, TSASN1Client } from "./TSASN1Client";
import { EASN1TransportEncoding } from "./TSInvokeContext";
import {
	CustomInvokeProblemEnum,
	ELogSeverity,
	ESocketState,
	IConnectionSocket,
	ISocketCloseEvent,
	ISocketConnectedEvent,
	ISocketErrorEvent,
	ISocketMessageEvent,
} from "./TSROSEBase";
import { ENetUC_Common } from "./types.ts";

/**
 * The ASN1 client side as required in node (different websocket and timer)
 */
export class TSASN1NodeClient extends TSASN1Client {
	// Our NODE websocket object
	private ws?: WebSocket;
	// The raw TCP socket (Which we currently only support in the node client)
	protected tcp?: net.Socket;
	// The reconnecting timer
	protected clientReconnectingTimeout?: ReturnType<typeof setTimeout>;

	/**
	 * Constructs the Client
	 *
	 * @param encoding - sets the encoding for outbound calls
	 */
	public constructor(encoding: EASN1TransportEncoding.JSON | EASN1TransportEncoding.BER) {
		super(encoding, ASN1ClassInstanceType.TSASN1NodeClient);
		this.onclose = this.onclose.bind(this);
		this.onerror = this.onerror.bind(this);
		this.onmessage = this.onmessage.bind(this);
		this.onconnected = this.onconnected.bind(this);
		this.send = this.send.bind(this);
		this.close = this.close.bind(this);
		this.clientReconnect = this.clientReconnect.bind(this);
	}

	/**
	 * Creates a node based websocket connection object and maps it into the IDualWebSocket interface
	 *
	 * @param address - The address to connect to
	 * @param options - Options as provided from the TSASN1Client
	 * @returns - the mapping socket object
	 */
	protected getConnectionSocket(address: string, options?: IWebSocketOptions): IConnectionSocket | undefined {
		if (this.connectionMode === EASNCONNECTIONMODE.WEBSOCKET) {
			this.ws = new WebSocket(address, options);
			this.ws.binaryType = "nodebuffer";
			const socket: IConnectionSocket = { readyState: this.ws.readyState, send: this.send, close: this.close };
			this.ws.onclose = (event: WebSocket.CloseEvent) => this.onclose(event, socket);
			this.ws.onerror = (event: WebSocket.ErrorEvent) => this.onerror(event, socket);
			this.ws.onmessage = (event: WebSocket.MessageEvent) => this.onmessage(event, socket);
			this.ws.onopen = (event: WebSocket.Event) => this.onconnected(event, socket);
			return socket;
		} else if (this.connectionMode === EASNCONNECTIONMODE.TCP) {
			const url = new URL(this.target);
			const host = url.hostname;
			const port = Number(url.port);
			const options: net.NetConnectOpts = { host, port };
			this.tcp = net.createConnection(options);
			this.tcp.on("close", (hadError: boolean) => this.onclose(hadError, socket));
			this.tcp.on("error", (err: Error) => this.onerror(err, socket));
			this.tcp.on("data", (data: Buffer) => this.onmessage(data, socket));
			this.tcp.on("connect", () => this.onconnected(undefined, socket));
			const socket: IConnectionSocket = { readyState: ESocketState.CONNECTING, send: this.send, close: this.close };
			return socket;
		} else {
			this.log(ELogSeverity.error, "Invalid connection mode", "getConnectionSocket", this, {
				mode: this.connectionMode,
			});
		}

		return undefined;
	}

	/**
	 * WebSocket onclose handler, handed over to the idual.onclose
	 *
	 * @param event - Node based CloseEvent
	 */
	private onclose(event: WebSocket.CloseEvent | boolean, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		let close: ISocketCloseEvent | undefined;
		if (this.ws) {
			const wsClose = event as WebSocket.CloseEvent;
			close = { code: wsClose.code, reason: wsClose.reason, wasClean: wsClose.wasClean, source: wsClose.target };
		} else if (this.tcp) {
			const hadError = event as boolean;
			close = { wasClean: !hadError, source: this.tcp };
		}
		if (close) {
			if (socket.onclose)
				socket.onclose(close);
			this.onClientClose(close);
		}
	}

	/**
	 * onerror handler for websocket and net sockets, handed over to the idual.onerror
	 *
	 * @param event - Node based ErrorEvent
	 */
	private onerror(event: WebSocket.ErrorEvent | Error, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		let error: ISocketErrorEvent | undefined;
		if (this.ws) {
			const wsError = event as WebSocket.ErrorEvent;
			error = { error: wsError.error, message: wsError.message, source: wsError.target, type: wsError.type };
		} else if (this.tcp) {
			const netError = event as Error;
			error = { error: netError.cause, message: netError.message, source: this.tcp };
		}
		if (error) {
			if (socket.onerror)
				socket.onerror(error);
			this.onClientError(error);
		}
	}

	/**
	 * WebSocket onmessage handler, handed over to the idual.onmessage
	 *
	 * @param event - Node based MessageEvent
	 */
	private onmessage(event: WebSocket.MessageEvent | Buffer, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		let message: ISocketMessageEvent | undefined;
		if (this.ws) {
			const wsEvent = event as WebSocket.MessageEvent;
			message = { data: wsEvent.data.toString(), type: wsEvent.type, source: wsEvent.target };
		} else if (this.tcp) {
			const netEvent = event as Buffer;
			message = { data: netEvent, source: this.tcp };
		}
		if (message) {
			if (socket.onmessage)
				socket.onmessage(message);
			this.onClientMessage(message);
		}
	}

	/**
	 * WebSocket onopen handler, handed over to the idual.onopen
	 *
	 * @param event - Node based OpenEvent
	 */
	private onconnected(event: WebSocket.Event | undefined, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		let open: ISocketConnectedEvent | undefined;
		if (this.ws) {
			const wsEvent = event as WebSocket.Event;
			open = { source: wsEvent.target, type: wsEvent.type };
		} else if (this.tcp) {
			open = { source: this.tcp };
		}
		if (open) {
			if (socket.onconnected)
				socket.onconnected(open);
			this.onClientConnected(open);
		}
	}

	/**
	 * Called to close the websocket. Calls the node based websocket close function
	 *
	 * @param code - Close code
	 * @param reason - Close reason
	 */
	private close(code?: number, reason?: string): void {
		if (this.ws)
			this.ws.close(code, reason);
		else if (this.tcp)
			this.tcp.destroy();
	}

	/**
	 * Called to send data through the websocket. Calls the node based websocket send function
	 *
	 * @param data - data to send through the websocket
	 */
	private send(data: string | Uint8Array): void {
		if (this.ws)
			this.ws.send(data);
		else if (this.tcp) {
			if (this.encoding == EASN1TransportEncoding.JSON) {
				// Only JSON needs a length header as the message itself contains no information about the length
				// BER shows struct and the length so we can distinguish from the beginning how long a message will be
				const len = data.length;
				if (len > 9999999) {
					const error = new ENetUC_Common.AsnRequestError({
						iErrorDetail: CustomInvokeProblemEnum.messageTooBig,
						u8sErrorString:
							`Could not generate JSON length header as message was too big. Maximum allows is 9999999, current size is ${len}`,
					});
					this.log(ELogSeverity.error, "Error creating JSON lenght header", "sendInvoke", this, undefined, error);
					throw error;
				}
				// Send the optional length prefix
				this.tcp.write(`J${String(len).padStart(7, "0")}`);
			}
			this.tcp.write(data);
		}
	}

	/**
	 * Wrapper for the node based fetch method
	 *
	 * @param input - The url we want to fetch data from
	 * @param init - A RequestInit object
	 * @returns - A Promise resolving into the fetch response
	 */
	protected async fetch(input: string, init?: RequestInit): Promise<Response> {
// IF NODE<22
		return (await fetch(input, init as FetchInit)) as unknown as Response;
// ELSE
		return fetch(input, init);
// ENDIF
	}

	/**
	 * Set a node based timer for the asn1ClientReconnect method with duration timeout
	 *
	 * @param timeout - The timeout after which asn1ClientReconnect is called
	 */
	protected setReconnectTimeout(timeout: number): void {
		if (this.clientReconnectingTimeout) {
			clearTimeout(this.clientReconnectingTimeout);
			this.clientReconnectingTimeout = undefined;
		}
		if (timeout && this.autoreconnect)
			this.clientReconnectingTimeout = setTimeout(this.clientReconnect, timeout);
	}

	/**
	 * Prepares data from the message event to the notastion we need (Differs between node and browser implementation)
	 *
	 * @param event - the message that contains the payload
	 * @returns data on successs or undefined on error
	 */
	protected async prepareData(event: ISocketMessageEvent): Promise<Uint8Array | object | undefined> {
		let rawData: Uint8Array | object | undefined;
		if (event.data instanceof Uint8Array)
			rawData = event.data;
		else if (typeof event.data === "string")
			rawData = JSON.parse(event.data) as object;
		else {
			this.log(ELogSeverity.error, "exception", "Received unhandled data", this);
			debugger;
		}
		return rawData;
	}

	/**
	 * Map the net node socket state to our ESocketState
	 *
	 * @param state - the net socket state
	 * @returns our ESocketState
	 */
	protected getSocketReadStateFromNodeState(state: net.SocketReadyState): ESocketState {
		switch (state) {
			case "opening":
				return ESocketState.CONNECTING;
			case "open":
				return ESocketState.OPEN;
			case "closed":
				return ESocketState.CLOSED;
			case "readOnly":
			case "writeOnly":
			default:
				return ESocketState.CLOSING;
		}
	}

	/**
	 * Updates the idual socket
	 *
	 * @param socket - the shared socket object which is handed over to the base class
	 */
	protected updateSocketState(socket: IConnectionSocket): void {
		if (this.ws)
			socket.readyState = this.ws.readyState;
		else if (this.tcp)
			socket.readyState = this.getSocketReadStateFromNodeState(this.tcp.readyState);
	}
}
