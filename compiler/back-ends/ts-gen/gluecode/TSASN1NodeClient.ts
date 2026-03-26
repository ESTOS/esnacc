// Centralised code for the TypeScript converters.
// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do not directly edit or modify the code as it is machine generated and will be overwritten with every compilation

// dprint-ignore-file
/* eslint-disable */

import net from "node:net";
import WebSocket from "ws";
import { ASN1ClassInstanceType } from "./TSASN1Base.js";
import { EASNCONNECTIONMODE, IWebSocketOptions, TSASN1Client } from "./TSASN1Client.js";
import { EASN1TransportEncoding } from "./TSInvokeContext.js";
import {
	CustomInvokeProblemEnum,
	ELogSeverity,
	ESocketState,
	IConnectionSocket,
	ISocketCloseEvent,
	ISocketConnectedEvent,
	ISocketErrorEvent,
	ISocketMessageEvent,
} from "./TSROSEBase.js";
import * as ENetUC_Common from "./ENetUC_Common.js";

/**
 * First byte constants used to discriminate between frame types.
 *
 * JSON frames always start with the ASCII character 'J' (0x4A).
 * ROSE frames start with a context-specific constructed ASN.1 tag (0xA1–0xA4),
 * corresponding to the four ROSE operation types defined in X.880.
 */
const FRAME_START_JSON = 0x4A; // ASCII 'J'
const FRAME_START_ROSE_INVOKE = 0xA1; // [1] IMPLICIT — ROSEInvoke
const FRAME_START_ROSE_RESULT = 0xA2; // [2] IMPLICIT — ROSEResult
const FRAME_START_ROSE_ERROR = 0xA3; // [3] IMPLICIT — ROSEError
const FRAME_START_ROSE_REJECT = 0xA4; // [4] IMPLICIT — ROSEReject

/** The fixed size (in bytes) of a JSON frame header: 'J' + 7 ASCII decimal digits. */
const JSON_HEADER_SIZE = 8;

/**
 * Determines whether a given byte is a valid ROSE top-level tag.
 * All four ROSE tags are context-specific constructed single-byte tags.
 */
function isRoseTag(byte: number): boolean {
	return byte === FRAME_START_ROSE_INVOKE
		|| byte === FRAME_START_ROSE_RESULT
		|| byte === FRAME_START_ROSE_ERROR
		|| byte === FRAME_START_ROSE_REJECT;
}

/**
 * The ASN1 client side as required in node (different websocket and timer)
 */
export class TSASN1NodeClient extends TSASN1Client {
	// Our NODE websocket object
	private ws?: WebSocket;
	// The raw TCP socket (Which we currently only support in the node client)
	private tcp?: net.Socket;
	// The reconnecting timer
	private reconnectTimeout?: ReturnType<typeof setTimeout>;
	/**
	 * Accumulates bytes that have arrived over the TCP socket but do not yet
	 * form a complete frame.  New incoming data is always appended here first;
	 * complete frames are sliced out and the remainder stays in this buffer.
	 *
	 * Initialised to an empty Buffer so we never have to null-check it.
	 */
	private pendingSocketData: Buffer = Buffer.alloc(0);

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
	 * WebSocket onclose handler, handed over to the socket.onSocketClose
	 *
	 * @param event - the websocket or the tcp.socket close event parameters
	 */
	private onclose(event: WebSocket.CloseEvent | boolean, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		if (!socket.onSocketClose)
			return;
		if (this.ws) {
			const wsClose = event as WebSocket.CloseEvent;
			const close: ISocketCloseEvent = {
				code: wsClose.code,
				reason: wsClose.reason,
				wasClean: wsClose.wasClean,
				source: wsClose.target,
			};
			socket.onSocketClose(close);
		} else if (this.tcp) {
			const hadError = event as boolean;
			const close: ISocketCloseEvent = { wasClean: !hadError, source: this.tcp };
			socket.onSocketClose(close);
		}
	}

	/**
	 * onerror handler for websocket and net sockets, handed over to the socket.onSocketError
	 *
	 * @param event - the websocket or the tcp.socket error event parameters
	 */
	private onerror(event: WebSocket.ErrorEvent | Error, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		if (!socket.onSocketError)
			return;
		if (this.ws) {
			const wsError = event as WebSocket.ErrorEvent;
			const error: ISocketErrorEvent = {
				error: wsError.error,
				message: wsError.message,
				source: wsError.target,
				type: wsError.type,
			};
			socket.onSocketError(error);
		} else if (this.tcp) {
			const netError = event as Error;
			const error: ISocketErrorEvent = { error: netError.cause, message: netError.message, source: this.tcp };
			socket.onSocketError(error);
		}
	}

	/**
	 * WebSocket onmessage handler, handed over to the idual.onmessage
	 *
	 * @param event - the websocket or the tcp.socket message event parameters
	 */
	private onmessage(event: WebSocket.MessageEvent | Buffer, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		if (!socket.onSocketMessage)
			return;

		if (this.ws) {
			const wsEvent = event as WebSocket.MessageEvent;
			const data = this.prepareData(wsEvent.data);
			if (data) {
				const msg: ISocketMessageEvent = { data, type: wsEvent.type, source: wsEvent.target };
				socket.onSocketMessage(msg);
			}
		} else if (this.tcp) {
			const netEvent = event as Buffer;
			const messages = this.handleFraming(netEvent);
			if (messages) {
				// As the framing might provide multple messages at once we need to handle them one after another
				for (const message of messages) {
					const msg: ISocketMessageEvent = { data: message, source: this.tcp };
					socket.onSocketMessage(msg);
				}
			}
		}
	}

	/**
	 * WebSocket onopen handler, handed over to the idual.onopen
	 *
	 * @param event - the websocket or the tcp.socket message connected parameters
	 */
	private onconnected(event: WebSocket.Event | undefined, socket: IConnectionSocket): void {
		this.updateSocketState(socket);
		if (!socket.onSocketConnected)
			return;

		if (this.ws) {
			const wsEvent = event as WebSocket.Event;
			const open: ISocketConnectedEvent = { source: wsEvent.target, type: wsEvent.type };
			socket.onSocketConnected(open);
		} else if (this.tcp) {
			const open: ISocketConnectedEvent = { source: this.tcp };
			socket.onSocketConnected(open);
		}
	}

	/**
	 * Called to close the websocket. Calls the node based websocket close function
	 *
	 * @param code - Close code
	 * @param reason - Close reason
	 */
	private async close(code?: number, reason?: string): Promise<void> {
		if (this.ws) {
			const ws = this.ws;
			this.ws.removeAllListeners();
			this.ws.close(code, reason);
			this.ws = undefined;
			await new Promise((resolve) => ws.once("close", resolve));
		}
		else if (this.tcp) {
			const tcp = this.tcp;
			this.tcp.removeAllListeners();
			this.tcp.destroy();
			this.tcp = undefined;
			await new Promise((resolve) => tcp.once("close", resolve));
		}
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
		return fetch(input, init);
	}

	/**
	 * Set a node based timer for the asn1ClientReconnect method with duration timeout
	 *
	 * @param timeout - The timeout after which asn1ClientReconnect is called
	 */
	protected setReconnectTimeout(timeout: number): void {
		if (this.reconnectTimeout) {
			clearTimeout(this.reconnectTimeout);
			this.reconnectTimeout = undefined;
		}
		if (timeout && this.autoreconnect)
			this.reconnectTimeout = setTimeout(this.clientReconnect, timeout);
	}

	/**
	 * Prepares data from the message event to the notation we need
	 *
	 * @param event - the message that contains the payload
	 * @returns data on successs or undefined on error
	 */
	protected prepareData(data: string | Buffer | ArrayBuffer | Buffer[]): Uint8Array | object | undefined {
		if (typeof data === "string")
			return JSON.parse(data) as object;
		else if (Buffer.isBuffer(data))
			return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
		else if (data instanceof ArrayBuffer)
			return new Uint8Array(data);
		else if (Array.isArray(data))
			return new Uint8Array(Buffer.concat(data));
		else {
			this.log(ELogSeverity.error, "exception", "Received unhandled data", this);
			debugger;
		}
		return undefined;
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

	/**
	 * Accepts a raw chunk of bytes from the TCP socket and attempts to extract
	 * one or more complete protocol frames from the accumulated buffer.
	 *
	 * The method supports two frame formats on the same connection, identified
	 * by the first byte of each frame:
	 *
	 *  • **JSON frames** — start with ASCII `J` (0x4A), followed by exactly
	 *    7 ASCII decimal digits that encode the payload length in bytes, then
	 *    the raw JSON payload.  Example header: `J0000120` → 120-byte payload.
	 *
	 *  • **ROSE / ASN.1 BER frames** — start with one of the four ROSE
	 *    context-specific constructed tags (0xA1–0xA4: Invoke, Result, Error,
	 *    Reject).  The tag is always a single byte.  It is followed by a BER
	 *    definite-form length field and then the value bytes.
	 *
	 * Because TCP is a stream protocol, a single call may deliver a partial
	 * frame, a complete frame, or several frames at once.  This method handles
	 * all three cases by buffering incomplete data across calls.
	 *
	 * Bytes that do not match a known frame-start marker are silently discarded
	 * until a recognised marker is found.  In practice this should never happen
	 * on a well-behaved TCP connection, but it provides a safe recovery path.
	 *
	 * @param data - The raw bytes received from the socket in this read event.
	 * @returns    An array of complete frame payloads (one entry per frame), or
	 *             an empty array if the accumulated data does not yet contain
	 *             one complete frame.
	 */
	protected handleFraming(data: Buffer): Uint8Array[] {
		// Append the new chunk to whatever was left from the previous call.
		this.pendingSocketData = Buffer.concat([this.pendingSocketData, data]);

		const completeFrames: Uint8Array[] = [];

		// Keep consuming frames from the front of the buffer until we either
		// run out of data or hit an incomplete frame.
		while (this.pendingSocketData.length > 0) {
			const firstByte = this.pendingSocketData[0]!;

			if (firstByte === FRAME_START_JSON) {
				// ── JSON frame ──────────────────────────────────────────────
				const frame = this.tryConsumeJsonFrame();
				if (!frame) {
					// Incomplete header or payload — wait for more data.
					break;
				}
				completeFrames.push(frame);
			} else if (isRoseTag(firstByte)) {
				// ── ROSE / ASN.1 BER frame ───────────────────────────────
				const frame = this.tryConsumeBerFrame();
				if (!frame) {
					// Incomplete TLV — wait for more data.
					break;
				}
				completeFrames.push(frame);
			} else {
				// ── Unknown / garbled byte ───────────────────────────────
				// Discard this byte and keep scanning.  On a clean TCP stream
				// this branch should never be reached.
				this.pendingSocketData = this.pendingSocketData.subarray(1);
			}
		}

		return completeFrames;
	}

	// -----------------------------------------------------------------------
	// Private helpers
	// -----------------------------------------------------------------------

	/**
	 * Attempts to consume one complete JSON frame from the front of
	 * `pendingSocketData`.
	 *
	 * Frame layout:
	 * ```
	 * ┌───┬─────────────────────────┬──────────────────────────┐
	 * │ J │  7 ASCII decimal digits │  <length> bytes of JSON  │
	 * └───┴─────────────────────────┴──────────────────────────┘
	 *   1             7                      variable
	 * ```
	 *
	 * @returns The complete frame (header + payload) as a `Uint8Array`, or
	 *          `undefined` if the buffer does not yet hold a full frame.
	 */
	private tryConsumeJsonFrame(): Uint8Array | undefined {
		// We need at least the 8-byte header before we can determine the
		// payload length.
		if (this.pendingSocketData.length < JSON_HEADER_SIZE)
			return undefined;

		// Parse the 7-digit decimal length field (bytes 1–7).
		const lengthString = this.pendingSocketData.toString("ascii", 1, JSON_HEADER_SIZE);
		const payloadLength = parseInt(lengthString, 10);

		// Guard against a malformed length field (e.g. non-numeric characters).
		// Should never happen on TCP, but keeps the parser robust.
		if (isNaN(payloadLength) || payloadLength < 0) {
			// Skip the 'J' byte and let the outer loop re-synchronise.
			this.pendingSocketData = this.pendingSocketData.subarray(1);
			return undefined;
		}

		const totalFrameLength = JSON_HEADER_SIZE + payloadLength;

		if (this.pendingSocketData.length < totalFrameLength) {
			// Payload hasn't arrived fully yet.
			return undefined;
		}

		// Slice out the complete frame and advance the pending buffer.
		const frame = new Uint8Array(
			this.pendingSocketData.buffer,
			this.pendingSocketData.byteOffset + JSON_HEADER_SIZE, // ← skips 8-byte header
			payloadLength, // ← payload length only
		);
		this.pendingSocketData = this.pendingSocketData.subarray(totalFrameLength);

		return frame;
	}

	/**
	 * Attempts to consume one complete ASN.1 BER frame from the front of
	 * `pendingSocketData`.
	 *
	 * Expected top-level structure (definite length only):
	 * ```
	 * ┌──────────┬──────────────────────────────┬───────────────┐
	 * │  Tag     │         Length field         │  Value bytes  │
	 * │  1 byte  │  1 byte (short) or 2–5 bytes │   variable    │
	 * └──────────┴──────────────────────────────┴───────────────┘
	 * ```
	 *
	 * BER definite length encoding:
	 *  - Short form  (0x00–0x7F): the byte itself is the length.
	 *  - Long form   (0x81–0x84): the low 7 bits tell how many subsequent
	 *    bytes encode the length as a big-endian unsigned integer.
	 *    We support up to 4 subsequent bytes (max ~4 GB), which is more
	 *    than sufficient for any real ROSE message.
	 *
	 * @returns The complete TLV frame as a `Uint8Array`, or `undefined` if the
	 *          buffer does not yet hold a full frame.
	 */
	private tryConsumeBerFrame(): Uint8Array | undefined {
		// Minimum: 1 tag byte + 1 length byte.
		if (this.pendingSocketData.length < 2)
			return undefined;

		// ── Tag ─────────────────────────────────────────────────────────────
		// All ROSE top-level tags fit in one byte (verified by the caller).
		const tagSize = 1;

		// ── Length ──────────────────────────────────────────────────────────
		const lengthResult = this.parseBerLength(tagSize);
		if (!lengthResult) {
			// Not enough bytes to parse the length field yet.
			return undefined;
		}

		const { valueLength, lengthFieldSize } = lengthResult;
		const totalFrameLength = tagSize + lengthFieldSize + valueLength;

		if (this.pendingSocketData.length < totalFrameLength) {
			// Value bytes haven't all arrived yet.
			return undefined;
		}

		// Slice out the complete TLV and advance the pending buffer.
		const frame = new Uint8Array(this.pendingSocketData.buffer, this.pendingSocketData.byteOffset, totalFrameLength);
		this.pendingSocketData = this.pendingSocketData.subarray(totalFrameLength);

		return frame;
	}

	/**
	 * Parses a BER definite-form length field starting at `offset` within
	 * `pendingSocketData`.
	 *
	 * Supported encodings:
	 * ```
	 *  Short form:  0xxxxxxx  — value is the byte itself (0–127)
	 *  Long  form:  1xxxxxxx  — low 7 bits = number of subsequent length bytes
	 *                           (we support 1–4 subsequent bytes)
	 * ```
	 *
	 * @param offset - Byte offset into `pendingSocketData` where the length field
	 *                 starts (i.e. directly after the tag bytes).
	 * @returns An object `{ valueLength, lengthFieldSize }` on success, or
	 *          `undefined` if there are not yet enough bytes to decode the field.
	 *          `lengthFieldSize` is the number of bytes consumed by the length
	 *          field itself (NOT including the value).
	 */
	private parseBerLength(offset: number): { valueLength: number; lengthFieldSize: number; } | undefined {
		if (this.pendingSocketData.length <= offset)
			return undefined;

		const firstLengthByte = this.pendingSocketData[offset]!;

		if ((firstLengthByte & 0x80) === 0) {
			// ── Short form ── single byte encodes the length directly.
			return { valueLength: firstLengthByte, lengthFieldSize: 1 };
		}

		// ── Long form ── low 7 bits tell us how many bytes follow.
		const numLengthBytes = firstLengthByte & 0x7F;

		// Guard: we only handle up to 4 subsequent length bytes.
		if (numLengthBytes === 0 || numLengthBytes > 4) {
			// 0x80 = indefinite form (not used in this protocol).
			// >4   = length value > 4 GB (not realistic).
			throw new Error(`Unsupported BER length encoding: 0x${firstLengthByte.toString(16).padStart(2, "0")}`);
		}

		// Check that all length bytes have arrived.
		if (this.pendingSocketData.length < offset + 1 + numLengthBytes)
			return undefined;

		// Read the multi-byte length as a big-endian unsigned integer.
		let valueLength = 0;
		for (let i = 0; i < numLengthBytes; i++)
			valueLength = (valueLength << 8) | this.pendingSocketData[offset + 1 + i]!;

		return { valueLength, lengthFieldSize: 1 + numLengthBytes };
	}
}
