// Centralised code for the TypeScript converters.
// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do not directly edit or modify the code as it is machine generated and will be overwritten with every compilation

// prettier-ignore

import { TSASN1Client } from "./TSASN1Client";
import { EASN1TransportEncoding, ELogSeverity, IDualWebSocket } from "./TSROSEBase";
import { ASN1ClassInstanceType } from "./TSASN1Base";
/**
 * The ASN1 client side as required in the browser (different websocket and timer)
 */
export abstract class TSASN1BrowserClient extends TSASN1Client {
	private idual: IDualWebSocket | null = null;
	private socket?: WebSocket;
	// The reconnecting timer
	private asn1ClientReconnectingTimeout?: number;

	/**
	 * Constructs the Client
	 *
	 * @param encoding - sets the encoding for outbound calls
	 */
	protected constructor(encoding: EASN1TransportEncoding.JSON | EASN1TransportEncoding.BER) {
		super(encoding, ASN1ClassInstanceType.TSASN1BrowserClient);
	}

	/**
	 * Creates a browser based websocket connection object and maps it into the IDualWebSocket interface
	 *
	 * @param address - The address to connect to
	 * @returns - A IDualWebSocket interface to map browser based websocket things into the IDualWebSocket interface or undefined if no websocket object was created
	 */
	protected getWebSocket(address: string): IDualWebSocket | undefined {
		if (typeof WebSocket !== "undefined")
			this.socket = new WebSocket(address);
		else if (typeof window !== "undefined")
			this.socket = new window.WebSocket(address);

		if (this.socket) {
			this.socket.onclose = this.onclose.bind(this);
			this.socket.onerror = this.onerror.bind(this);
			this.socket.onmessage = this.onmessage.bind(this);
			this.socket.onopen = this.onopen.bind(this);
			this.idual = {
				readyState: this.socket.readyState,
				onclose: null,
				onerror: null,
				onmessage: null,
				onopen: null,
				send: this.send.bind(this),
				close: this.close.bind(this),
				addEventListener: this.addEventListener.bind(this),
				removeEventListener: this.removeEventListener.bind(this)
			};
			return this.idual;
		}

		return undefined;
	}

	/**
	 * WebSocket onclose handler, handed over to the idual.onclose
	 *
	 * @param event - Broser based CloseEvent
	 */
	private onclose(event: CloseEvent): void {
		if (this.idual && this.socket)
			this.idual.readyState = this.socket.readyState;
		if (this.idual && this.idual.onclose)
			this.idual.onclose(event);
	}

	/**
	 * WebSocket onerror handler, handed over to the idual.onerror
	 *
	 * @param event - Broser based Event
	 */
	private onerror(event: Event): void {
		if (this.idual && this.socket)
			this.idual.readyState = this.socket.readyState;
		if (this.idual && this.idual.onerror) {
			this.idual.onerror({
				error: event,
				message: "",
				type: event.type,
				target: event.target
			});
		}
	}

	/**
	 * WebSocket onmessage handler, handed over to the idual.onmessage
	 *
	 * @param event - Broser based MessageEvent
	 */
	private onmessage(event: MessageEvent<string>): void {
		if (this.idual && this.socket)
			this.idual.readyState = this.socket.readyState;
		if (this.idual && this.idual.onmessage) {
			this.idual.onmessage({
				data: event.data.toString(),
				type: event.type,
				target: event.target
			});
		}
	}

	/**
	 * WebSocket onopen handler, handed over to the idual.onopen
	 *
	 * @param event - Broser based Event
	 */
	private onopen(event: Event): void {
		if (this.idual && this.socket)
			this.idual.readyState = this.socket.readyState;
		if (this.idual && this.idual.onopen)
			this.idual.onopen(event);
	}

	/**
	 * Called to close the websocket. Calls the browser based websocket close function
	 *
	 * @param code - Close code
	 * @param reason - Close reason
	 */
	private close(code?: number, reason?: string): void {
		if (this.socket)
			this.socket.close(code, reason);
	}

	/**
	 * Called to send data through the websocket. Calls the browser based websocket send function
	 *
	 * @param data - data to send through the websocket
	 */
	private send(data: string): void {
		if (this.socket)
			this.socket.send(data);
	}

	/**
	 * Allows to add an event handler for dedicated websocket events
	 *
	 * @param type - type of event the listener wants to subscribe to
	 * @param listener - the listener wanted to get informed about the event
	 */
	private addEventListener(type: "close" | "error" | "message" | "open", listener: () => void): void {
		if (this.socket)
			this.socket.addEventListener(type, listener);
	}

	/**
	 * Allows to remove an event handler from dedicated websocket events
	 *
	 * @param type - type of event the listener wants to unsubscribe from
	 * @param listener - the listener that wants to unsubscribe
	 */
	private removeEventListener(type: "close" | "error" | "message" | "open", listener: () => void): void {
		if (this.socket)
			this.socket.removeEventListener(type, listener);
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
	 * Set a browser based timer for the asn1ClientReconnect method with duration timeout
	 *
	 * @param timeout - The timeout after which asn1ClientReconnect is called
	 */
	protected asn1ClientsetReconnectTimeout(timeout: number): void {
		if (this.asn1ClientReconnectingTimeout) {
			window.clearTimeout(this.asn1ClientReconnectingTimeout);
			this.asn1ClientReconnectingTimeout = undefined;
		}
		if (timeout && this.autoreconnect)
			this.asn1ClientReconnectingTimeout = window.setTimeout(this.asn1ClientReconnect.bind(this), timeout);
	}

	/**
	 * Prepares data from the message event to the notastion we need (Differs between node and browser implementation)
	 *
	 * @param event - the message that contains the payload
	 * @returns data on successs or undefined on error
	 */
	protected async prepareData(event: MessageEvent): Promise<Uint8Array | object | undefined> {
		let rawData: Uint8Array | object | undefined;
		if (event.data instanceof Blob)
			rawData = new Uint8Array(await event.data.arrayBuffer());
		else if (event.data instanceof Uint8Array)
			rawData = event.data;
		else if (typeof event.data === "string")
			rawData = JSON.parse(event.data) as object;
		else {
			this.log(ELogSeverity.error, "exception", "Received unhandled data", this);
			debugger;
		}
		return rawData;
	}
}
