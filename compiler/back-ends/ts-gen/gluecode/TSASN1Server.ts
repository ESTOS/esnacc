// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do NOT edit or modify this code as it is machine generated
// and will be overwritten with every code generation of the esnacc.exe

// dprint-ignore-file
/* eslint-disable */

import { InvokeProblemenum, ROSEError, ROSEMessage, ROSEReject, ROSEResult } from "./SNACCROSE";
import { ASN1ClassInstanceType, PendingInvoke, TSASN1Base } from "./TSASN1Base";
import { EASN1TransportEncoding } from "./TSInvokeContext";
import {
	createInvokeReject,
	CustomInvokeProblemEnum,
	ELogSeverity,
	IASN1InvokeData,
	IASN1Transport,
	ReceiveInvokeContext,
	ROSEBase,
} from "./TSROSEBase";

/**
 * Interface a client connection object has to provide to be able to send an event based on the id of a client to this client
 */
export interface IASN1ClientConnection {
	send(response: string | Uint8Array): boolean;
	getTransportEncoding(): EASN1TransportEncoding | undefined;
	setTransportEncoding(encoding: EASN1TransportEncoding): void;
}

/**
 * Interface a client connection handler has to provide to be able to fetch a dedicated client connections based on the id of a client
 */
export interface IASN1ClientConnectionHandler {
	getClientConnection(id: string): IASN1ClientConnection | undefined;
}

/**
 * The TSASN1Server implements the ROSE server side
 */
export class TSASN1Server extends TSASN1Base implements IASN1Transport {
	// The Client connection handler that is capable of providing a client connection based on the id of that client
	private connectionhandler?: IASN1ClientConnectionHandler = undefined;

	/**
	 * Constructs the server
	 *
	 * @param encoding - sets the encoding for outbound calls
	 */
	public constructor(encoding: EASN1TransportEncoding.JSON | EASN1TransportEncoding.BER) {
		super(encoding, ASN1ClassInstanceType.TSASN1Server);
	}

	/**
	 * The server has to set the clientconnection handler implementing the IASN1ClientConnectionHandler interface
	 * The server will ask this handler for a connection object based on the sessionID in the ROSEInvoke object
	 * The handler searches for the client connection and passes it back.
	 * The server will send the event via the IASN1ClientConnection interface.
	 *
	 * @param handler - A class or object implementing the IASN1ClientConnectionHandler
	 */
	public setClientConnectionHandler(handler: IASN1ClientConnectionHandler): void {
		this.connectionhandler = handler;
	}

	/**
	 * Sends an event synchronous to a client
	 *
	 * @param data - the data for the invoke
	 * @returns true when the event was sent
	 */
	public sendEventSync(data: IASN1InvokeData): boolean {
		if (this.connectionhandler && data.invokeContext.clientConnectionID) {
			const client = this.connectionhandler.getClientConnection(data.invokeContext.clientConnectionID);
			if (client) {
				const encoded = ROSEBase.encodeToTransport(data.payLoad, this.encodeContext);
				client.send(encoded.payLoad);
				return true;
			}
		}
		return false;
	}

	/**
	 * Retrieves the client connection ID we got from the server
	 *
	 * @returns the client connection ID (ROSE sessionID)
	 */
	public getSessionID(): undefined {
		return undefined;
	}

	/**
	 * Gets the encoding from the transport
	 * This method is overwritten in the server to get the encoding of a certain client connection
	 *
	 * @param clientConnectionID - the clientConnectionID for which we want to get the encoding
	 * @returns - the encoding for a certain clientConnectionID or the default encoding of this instance
	 */
	public override getEncoding(clientConnectionID?: string): EASN1TransportEncoding {
		let result: undefined | EASN1TransportEncoding;
		if (clientConnectionID)
			result = this.connectionhandler?.getClientConnection(clientConnectionID)?.getTransportEncoding();
		if (result)
			return result;
		return super.getEncoding();
	}

	/**
	 * Sets the encoding to the transport
	 * This method is overwritten in the server to set the encoding of a certain client connection
	 *
	 * @param encoding - the encoding we want to set
	 * @param clientConnectionID - the clientConnectionID for which we want to set the encoding
	 */
	public override setEncoding(encoding: EASN1TransportEncoding, clientConnectionID?: string): void {
		if (clientConnectionID)
			this.connectionhandler?.getClientConnection(clientConnectionID)?.setTransportEncoding(encoding);
		else {
			super.setEncoding(encoding);
			return;
		}
	}

	/**
	 * Sends a message towards the client
	 * Creates a completion object to associate request with response.
	 * The callback is handed over to the completion object and called out of it
	 * For and event no timeout or callback is used or required (invokeID already set to 99999 for an event while calling)
	 * Only invokes are supported and they need to contain the sessionid for the target client
	 * The external connectionhandler has to provides the proper client for the sessionid
	 *
	 * @param data - the data for the invoke
	 * @returns - a Promise based ROSE message (reject, result, error) as provided by the other side (or in case of timeout).
	 * If no timeout was specified we resolve in undefined to cleanup the promise object
	 */
	public async sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		const clientConnectionID = data.invokeContext.clientConnectionID || data.invoke.sessionID;
		if (!clientConnectionID) {
			return createInvokeReject(
				data.invoke.invokeID,
				InvokeProblemenum.invalidSessionID,
				"SendInvoke no sessionID specified",
			);
		}
		else if (!this.connectionhandler) {
			return createInvokeReject(
				data.invoke.invokeID,
				InvokeProblemenum.unrecognisedOperation,
				"SendInvoke connectionhandler not set. Call setClientConnectionHandler",
			);
		}

		const client = this.connectionhandler.getClientConnection(clientConnectionID);
		if (!client) {
			return createInvokeReject(
				data.invoke.invokeID,
				InvokeProblemenum.invalidSessionID,
				`SendInvoke no client for sessionID ${clientConnectionID}`,
			);
		}

		return new Promise((resolve, reject): void => {
			let resolveUndefined = true;
			const receiveInvokeContext = ReceiveInvokeContext.create(data.invoke);

			try {
				if (data.invoke.invokeID !== 99999) {
					const timeout = data.invokeContext?.timeout || this.defaultTimeout;
					// Create a timer if a timeout was provided
					const id = data.invoke.invokeID;
					const timerid = setTimeout((): void => {
						this.onROSETimeout(id);
					}, timeout);
					const pending = new PendingInvoke(data.invoke, resolve, reject, timerid);
					this.pendingInvokes.set(id, pending);
					resolveUndefined = false;
				}
				const encoded = ROSEBase.encodeToTransport(data.payLoad, this.encodeContext);

				this.logTransport(encoded.logData, "sendInvoke", "out", data.invokeContext);
				if (!client.send(encoded.payLoad)) {
					const msg = new ROSEMessage();
					msg.reject = {
						invokedID: { invokedID: data.invoke.invokeID },
						sessionID: data.invoke.sessionID,
						reject: { invokeProblem: CustomInvokeProblemEnum.serviceUnavailable },
						details: `SendInvoke to client with sessionID ${data.invoke.sessionID} failed`,
					};
					this.onROSEReject(msg.reject, receiveInvokeContext);
					this.log(ELogSeverity.error, "sending failed", "sendInvoke", this, msg.reject);
				}
			}
			catch (error) {
				this.log(ELogSeverity.error, "sending failed with exception", "sendInvoke", this, undefined, error);
				resolve(
					createInvokeReject(
						data.invoke.invokeID,
						InvokeProblemenum.resourceLimitation,
						`SendInvoke catched an unhandled exception ${data.invoke.sessionID}`,
					),
				);
			}
			if (resolveUndefined)
				resolve(undefined);
		});
	}
}
