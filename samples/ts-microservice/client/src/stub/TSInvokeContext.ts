// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do NOT edit or modify this code as it is machine generated
// and will be overwritten with every code generation of the esnacc.exe

// dprint-ignore-file
/* eslint-disable */

import { IncomingHttpHeaders } from "node:http";

/**
 * Handles http headers on the receiving and sending side.
 * Allows direct access to a name indexed object
 */
export type EHttpHeaders = IncomingHttpHeaders;

/**
 * What is the encoding for sending a message, in which encoding did we receive a message
 */
export enum EASN1TransportEncoding {
	JSON = 1,
	BER = 2,
}

/**
 * Base params for the invoke context
 */
export interface IInvokeContextBaseParams {
	// Encoding of the message
	encoding?: EASN1TransportEncoding;
	// The connection id under which the request was received send
	// For websocket this is the server defined websocket id (WS_xxx) for a rest request the client generated id (CL_xxx)
	clientConnectionID?: string;
	// The header elements that were provided along with the request or shall be send
	headers?: EHttpHeaders;
	// custom (void*) to store own data in the invokecontext
	customData?: unknown;
}

/**
 * The receive invoke Context allows handing over invoke relevant data from the receiving layer into the handling layer
 * e.g. you would set the session id in here to know on behalf of which session the invoke was received
 * or a websocket to know the websocket the request was received with
 */
export interface IReceiveInvokeContextParams extends IInvokeContextBaseParams {
	// Client IP for logging (the only identifier we have in a rest request)
	clientIP?: string;

	// If this flag is set to true we handle an event, even if handleEvents in the ROSE class is set to false
	// If handleEvents is set to false we need to overwrite it to dispatch the queued events.
	handleEvent?: true;

	// in case the request was handled by rest we add the url here
	// The url may contain the operationName in case the body is not containing the full ROSE message
	url?: string;

	// The request was received without a ROSE envelop
	// This value is only available on the server side if a request was sent without a ROSE envelop as HTTP rest like request
	noROSEEnvelop?: true;
}

export interface ISendInvokeContextParams extends IInvokeContextBaseParams {
	// The timeout value the caller wants to wait for a result
	timeout?: number;

	// Defines the REST-Target for the request (allows to use websocket and rest simultaneously)
	// Specify a https://rest endpoint
	// If no target is specified the handler will use the target that has been specified using setTarget
	restTarget?: string;

	// Allows to send an event synchronous if we are in an established websocket connection
	// Will fail if the websocket is not established or not in an ready state
	// Has no effect on invokes
	bSendEventSynchronous?: boolean;

	// We technically do not need the operationName in the ROSEInvoke message so adding it is optional
	// by default the caller is not sending the operationName as it is not really needed
	bAddOperationName?: boolean;
}
