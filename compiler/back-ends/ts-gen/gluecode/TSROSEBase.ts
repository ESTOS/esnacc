// Centralised code for the TypeScript converters.
// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do not directly edit or modify the code as it is machine generated and will be overwritte n with every compilation

// prettier-ignore
/* eslint-disable no-debugger */

import { InvokeProblemenum, RejectProblem, ROSEError, ROSEInvoke, ROSEMessage, ROSEReject, ROSERejectChoice, ROSEResult, ROSEResultSeq } from "./SNACCROSE";
import { ConverterErrors, DecodeContext, IConverter, EncodeContext } from "./TSConverterBase";
import { ENetUC_Common, ENetUC_Common_Converter } from "./types";
import { IncomingHttpHeaders } from "http";
import { ROSEMessage_Converter, ROSEReject_Converter } from "./SNACCROSE_Converter";
import * as asn1ts from "@estos/asn1ts";

/**
 * The websocket is different between the node and the browser implemenation, thus we cast it to any
 */
// eslint-disable-next-line @typescript-eslint/no-explicit-any
type IOriginalWebSocket = any

// Special Custom Invoke Problems for internal usage (used in Error Responses)
export enum CustomInvokeProblemEnum {
	// HTTP - no response (NGINX)
	missingResponse = 444,
	// HTTP - Service Unavailable
	serviceUnavailable = 503,
	// HTTP - Gateway Timeout
	requestTimedOut = 504,
	// HTTP - internal error (e.g. if we fail after parsing the argument)
	internalError = 500
}

/**
 * As the websocket implementation in node and the browser differ we need an abstraction layer that
 * fullfills both sides and is implemented according the native websocket implementation in TSASN1BrowserClient or TSASN1NodeClient
 * Encapsulates methods and events for both sides.
 */
export interface IDualWebSocketCloseEvent {
	code: number;
	wasClean: boolean;
	reason: string;
	target: IOriginalWebSocket;
}

export interface IDualWebSocketErrorEvent {
	error: unknown;
	message: string;
	type: string;
	target: IOriginalWebSocket;
}

export interface IDualWebSocketMessageEvent {
	data: string | Uint8Array;
	type: string;
	target: IOriginalWebSocket;
}

export interface IDualWebSocketOpenEvent {
	target: IOriginalWebSocket;
}

export enum EDualWebSocketState {
	CONNECTING = 0,
	OPEN = 1,
	CLOSING = 2,
	CLOSED = 3
}

export interface IDualWebSocket {
	readyState: EDualWebSocketState;
	onclose: ((ev: IDualWebSocketCloseEvent) => void) | null;
	onerror: ((ev: IDualWebSocketErrorEvent) => void) | null;
	onmessage: ((ev: IDualWebSocketMessageEvent) => void) | null;
	onopen: ((ev: IDualWebSocketOpenEvent) => void) | null;
	close(code?: number, reason?: string): void;
	// Also supports blob but we use either string or Uint8Array internally
	send(data: string | Uint8Array /* | Blob */): void;
	// eslint-disable-next-line @typescript-eslint/no-explicit-any
	addEventListener(type: string, listener?: (event: any) => unknown): void;
	// eslint-disable-next-line @typescript-eslint/no-explicit-any
	removeEventListener(type: string, listener?: (event: any) => unknown): void;
}

/**
 * Centralized method that creates a ROSEReject for an invoke problem
 *
 * @param invoke - The invoke this reject is created for or the number
 * @param value - The invoke problem enum value
 * @param description - An additional description to hand over to the other side
 * @returns - the created ROSEReject object
 */
export function createInvokeReject(invoke: ROSEInvoke | number, value: InvokeProblemenum | CustomInvokeProblemEnum, description?: string): ROSEReject {
	let sessionID: string | undefined;
	let invokedID: number;
	if (typeof invoke === "number")
		invokedID = invoke;
	else {
		invokedID = invoke.invokeID;
		sessionID = invoke.sessionID;
	}
	return new ROSEReject({
		invokedID: new ROSERejectChoice({
			invokedID
		}),
		sessionID,
		reject: new RejectProblem({
			invokeProblem: value
		}),
		details: description
	});
}

/**
 * This object is used to cache an event the client is not able to handle (Session negotiation, cache events until we are ready to handle them)
 */
export interface ICachedEvent {
	invoke: ROSEInvoke;
	invokeContext: IReceiveInvokeContext;
	handler: unknown;
}

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
	BER = 2
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
 * Base interface for the invoke context
 */
export interface IInvokeContextBase extends IInvokeContextBaseParams {
	// Set the base ids in one call
	init: (operationID: number, operationName: string, invokeID?: number) => void;
	// Which operation id has been called
	operationID: number;
	// Which operation name has been called
	operationName: string;
	// The invokeID the client provided
	invokeID: number;
}

/**
 * A class holding the properties of the IInvokeContextBase
 */
class BaseInvokeContext implements IInvokeContextBase {
	public encoding: EASN1TransportEncoding | undefined;
	public operationID: number;
	public operationName: string;
	public invokeID: number;
	public clientConnectionID: string | undefined;
	public headers: EHttpHeaders | undefined;
	public customData: unknown | undefined;

	/**
	 * Constructor for the BaseInvokeContext
	 *
	 * @param args - Arguments the object will be initialized with, if a mandatory one is missing we prefill with default values
	 */
	public constructor(args: Partial<IInvokeContextBase> | undefined) {
		this.encoding = args?.encoding;
		this.clientConnectionID = args?.clientConnectionID;
		this.headers = args?.headers;
		this.operationID = args?.operationID || -1;
		this.operationName = args?.operationName || "";
		this.invokeID = args?.invokeID || -1;
		this.customData = args?.customData;
	}

	/**
	 * Allows to set the baseIDs for an invoke (inbound or outbound) in one call
	 *
	 * @param operationID - the rose operationID
	 * @param operationName - the rose operationName
	 * @param invokeID - the rose invokeID
	 */
	public init(operationID: number, operationName: string, invokeID?: number): void {
		this.operationID = operationID;
		this.operationName = operationName;
		if (invokeID)
			this.invokeID = invokeID;
	}
}

/**
 * The receive invoke Context allows handing over invoke relevant data from the receiving layer into the handling layer
 * e.g. you would set the session id in here to know on behalf of which session the invoke was received
 * or a websocket to know the websocket the request was received with
 */
export interface IReceiveInvokeContextParams {
	// Client IP for logging (the only identifier we have in a rest request)
	clientIP?: string;

	// If this flag is set to true we handle an event, even if handleEvents in the ROSE class is set to false
	// If handleEvents is set to false we need to overwrite it to dispatch the queued events.
	handleEvent?: true;
}
export interface IReceiveInvokeContext extends IReceiveInvokeContextParams, IInvokeContextBase {
}

/**
 * A class holding the properties of the IReceiveInvokeContext
 */
export class ReceiveInvokeContext extends BaseInvokeContext implements IReceiveInvokeContextParams {
	public clientIP?: string;
	public handleEvent?: true;
	/**
	 * Constructor for the ReceiveInvokeContext
	 *
	 * @param args - Arguments the object will be initialized with, if a mandatory one is missing we prefill with default values
	 */
	public constructor(args?: Partial<IReceiveInvokeContext>) {
		super(args);
		this.clientIP = args?.clientIP;
		this.handleEvent = args?.handleEvent;
	}

	/**
	 * Creates a ReceiveInvokeContext based on a ROSEInvoke object
	 *
	 * @param invoke - the ROSEInvoke object
	 * @returns the ReceiveInvokeContext object
	 */
	public static create(invoke: ROSEInvoke): ReceiveInvokeContext {
		return new ReceiveInvokeContext({
			clientConnectionID: invoke.sessionID,
			invokeID: invoke.invokeID,
			operationID: invoke.operationID,
			operationName: invoke.operationName
		});
	}
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
}

/**
 * The send invoke Context allows handing over event relevant data from the handling layer to the sending layer
 * e.g. you would set the session id in here to define over which connection the transport shall send the request
 */
export interface ISendInvokeContext extends ISendInvokeContextParams, IInvokeContextBase {
}

/**
 * A class holding the properties of the ISendInvokeContext
 */
export class SendInvokeContext extends BaseInvokeContext implements ISendInvokeContextParams {
	public timeout?: number;
	public restTarget?: string;
	public bSendEventSynchronous?: boolean;

	/**
	 * Constructor for the SendInvokeContext
	 *
	 * @param args - Arguments the object will be initialized with, if a mandatory one is missing we prefill with default values
	 */
	public constructor(args: Partial<ISendInvokeContext>) {
		super(args);
		this.timeout = args.timeout;
		this.restTarget = args.restTarget;
		this.bSendEventSynchronous = args.bSendEventSynchronous;
	}
}

type AsnInvokeProblemEnum = InvokeProblemenum;
export { InvokeProblemenum as AsnInvokeProblemEnum };

/**
 * The log data blob whichn is provided through the ILogCallback
 */
export interface IASN1LogData {
	className: string;
	classProps?: {
		// This object will contain further application and implementation specific members like
		// session ids, user ids etc.
		[propName: string]: unknown;
	};
}

/**
 * The callback that is used to query log data for a log invoke
 */
export interface IASN1LogCallback {
	getLogData(): IASN1LogData;
}

/**
 * Details for an invoke problem
 */
export class AsnInvokeProblem {
	public value?: AsnInvokeProblemEnum;
	public details = "";

	/**
	 * Constructs an invoke problem object, simply stores the handed over arguments in the class
	 *
	 * @param value - The invoke problem enum value
	 * @param details - a readable error description for debugging and diagnostics
	 */
	public constructor(value: AsnInvokeProblemEnum, details?: string) {
		this.value = value;
		if (details != null)
			this.details = details;
	}
}

/**
 * Centralized method that creates an AsnInvokeProblem out of a ROSEReject Message
 *
 * @param roseReject - The rose reject object
 * @returns - The created AsnInvokeProblem object
 */
export function handleRoseReject(roseReject: ROSEReject): AsnInvokeProblem {
	let value = 0xffffffff;

	if (roseReject.reject) {
		const reject = roseReject.reject;
		if (reject.invokeProblem)
			value = reject.invokeProblem;
		else {
			// The Typescript Code will only return invokeProblems.
			// All others are matched here to have them available but will not be used in a fromto typescript application
			if (reject.generalProblem)
				value = reject.generalProblem + 0xff;
			else if (reject.returnErrorProblem)
				value = reject.returnErrorProblem + 0xffff;
			else if (reject.returnResultProblem)
				value = reject.returnResultProblem + 0xffffff;
		}
	}

	return new AsnInvokeProblem(value, roseReject.details);
}

/**
 * Function throws an exception if the handed over object is not a dedicated but a plain object
 * e.g. if someone calls the function with a \{ \} object that fullfills the requirements but actually is not an object created with new
 *
 * @param obj - The object to test
 */
export function validateIsDedicatedObject(obj: unknown): void {
	if (Object.prototype.isPrototypeOf.call(Object.getPrototypeOf(obj), Object)) {
		debugger;
		// This stub relies on using constructor created objects as we use instanceof in the stub
		// Thus an object created with {} is not usable here as it fails with the instanceof checks
		// Simply pass the parameters you have in your {} into the argument of the constructor any everything is fine...
		throw new Error("You must use this stub with dedicated constructor created object and not interface like objects!");
	}
}
/**
 * Defines the severity of the log message
 */
export enum ELogSeverity {
	error = 1,
	warn = 2,
	info = 3,
	debug = 4
}

/**
 * Defines the interface the logger has to fullfill in order to handle log messages from the rose stub
 */
export interface IROSELogger {
	error(msg: string, calling_method: string, logData?: IASN1LogData, meta?: unknown, exception?: unknown): void;
	warn(msg: string, calling_method: string, logData?: IASN1LogData, meta?: unknown, exception?: unknown): void;
	info(msg: string, calling_method: string, logData?: IASN1LogData, meta?: unknown, exception?: unknown): void;
	debug(msg: string, calling_method: string, logData?: IASN1LogData, meta?: unknown, exception?: unknown): void;
}

/**
 * Defines the interface the InvokeHandler layer has to fullfill
 */
export interface IInvokeHandler {
	getNameForOperationID(id: number): string;
	onInvoke(invoke: ROSEInvoke, invokeContext: IReceiveInvokeContext, requesthandler: unknown): Promise<ROSEReject | ROSEResult | ROSEError | undefined>;
}

/**
 * Defines the interface the transport layer has to fullfill
 */
export interface IASN1Transport {
	sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined>;
	sendEvent(data: IASN1InvokeData): undefined | boolean;
	registerOperation(oninvokehandler: IInvokeHandler, requesthandler: unknown, operationID: number): void;
	getEncodeContext(): EncodeContext;
	getDecodeContext(): DecodeContext;
	getNextInvokeID(): number;
	getSessionID(): string | undefined;
	getEncoding(clientConnectionID?: string): EASN1TransportEncoding;
	log(severity: ELogSeverity, message: string, calling_method: string, callback: IASN1LogCallback, meta?: unknown, exception?: unknown): void;
	logInvoke(calling_method: string, callback: IASN1LogCallback, argument: object, context: IReceiveInvokeContext | ISendInvokeContext, isOutbound: boolean): void;
	logResult(calling_method: string, callback: IASN1LogCallback, result: object, context: IReceiveInvokeContext | ISendInvokeContext, isOutbound: boolean): void;
	logError(calling_method: string, callback: IASN1LogCallback, error: object, invoke: unknown | undefined, context: IInvokeContextBase, isOutbound: boolean): void;
	logReject(calling_method: string, callback: IASN1LogCallback, reject: object, invoke: unknown | undefined, context: IInvokeContextBase, isOutbound: boolean): void;
}

// Envelop to be able to call initEmpty on the class object we pass to ROSEBase.handleRequest
interface IASN1DataClass {
	initEmpty(): unknown;
	readonly type: string;
}
// Envelop that holds the handler class that acts on invokes and events
interface IASN1HandlerClass {
	setLogContext?(argument: unknown, invokeContext: IReceiveInvokeContext): void;
}
// Type declares for methods that are handling a request
// eslint-disable-next-line @typescript-eslint/no-explicit-any
type IOnInvokeMethod = (argument: any, invokeContext: IReceiveInvokeContext) => Promise<object | ENetUC_Common.AsnRequestError | undefined>;
// eslint-disable-next-line @typescript-eslint/no-explicit-any
type IOnEventMethod = (argument: any, invokeContext: IReceiveInvokeContext) => void;

// Reference to the encoded ROSE message data (allows the function to fill in the encoded and hand it back to the caller)
export interface IASN1InvokeData {
	invoke: ROSEInvoke;
	logMessage: ROSEMessage | Uint8Array;
	payLoad: Uint8Array | string;
	context: SendInvokeContext;
}

/**
 * Decodes the argument and hands back hte
 *
 * @param argument - the argument to decode
 * Uint8Array - raw BER data
 * asn1ts.Sequence - an already ber decoded asn1ts object
 * string - raw JSON data
 * object - an already json decoded object
 * @param converter - the converter to use
 * @param errors - the list of errors if errors have occured
 * @param decodeContext - the context from the transport how to decode an object
 * @param invokeContext - the context that holds and receives the encoding
 * @returns the decoded object or undefined
 */
export function asn1Decode<T>(argument: Uint8Array | asn1ts.Sequence | string | object | undefined, converter: IConverter, errors: ConverterErrors, decodeContext: DecodeContext, invokeContext?: Partial<IInvokeContextBase>): T | undefined {
	let jsonData: object | string | undefined;
	let berData: Uint8Array | undefined;

	// We don't know the encoding yet -> lets try to determine it
	if (invokeContext?.encoding === undefined) {
		if (argument instanceof Uint8Array) {
			// It's raw data from the transport -> could be anything let's check JSON first if the first char is a { or [ afterwards BER
			if (argument[0] === 123 || argument[0] === 91) {
				// This could be JSON but may also be BER
				jsonData = argument.toString();
				berData = argument;
			} else
				berData = argument;
		} else if (typeof argument === "string")
			jsonData = argument;
		else if (argument instanceof asn1ts.Sequence) {
			// If this is an encapsulated object we need to use the raw data.
			// The encapsulated uses a certain scheme and we need to validate that scheme against the data we received
			berData = argument.valueBeforeDecodeView;
		} else if (typeof argument === "object")
			jsonData = argument;
		else
			debugger;
		// UCWeb creates an array envelop which is technically wrong so we need to remove that here
		// All ROSE messages are single sequences
		if (typeof jsonData === "string" && jsonData.startsWith("["))
			jsonData = jsonData.substring(1, jsonData.length - 1);
		else if (Array.isArray(jsonData) && jsonData.length === 1)
			jsonData = jsonData[0] as object;
	} else if (invokeContext.encoding === EASN1TransportEncoding.BER) {
		if (argument instanceof Uint8Array)
			berData = argument;
		else if (argument instanceof asn1ts.Sequence) {
			// If this is an encapsulated object we need to use the raw data.
			// The encapsulated uses a certain scheme and we need to validate that scheme against the data we received
			berData = argument.valueBeforeDecodeView;
		} else
			debugger;
	} else if (invokeContext.encoding === EASN1TransportEncoding.JSON) {
		if (argument instanceof Uint8Array)
			jsonData = argument.toString();
		else {
			const type = typeof argument;
			if (type === "string")
				jsonData = argument;
			else if (type === "object")
				jsonData = argument;
			else
				debugger;
		}
	}

	if (jsonData) {
		const result = converter.fromJSON(jsonData, errors, decodeContext);
		if (result) {
			if (invokeContext)
				invokeContext.encoding = EASN1TransportEncoding.JSON;
			return result as T;
		}
	}
	if (berData) {
		const result = converter.fromBER(berData, errors, decodeContext);
		if (result) {
			if (invokeContext)
				invokeContext.encoding = EASN1TransportEncoding.BER;
			return result as T;
		}
	}

	return undefined;
}

/**
 * Decodes the argument and hands back hte
 *
 * @param encoding - the encoding from the session or unknown
 * @param argument - the object to encode
 * @param converter - the converter to use
 * @param errors - the list of errors if errors have occured
 * @param encodeContext - the context from the transport how to decode an object
 * @returns the encoded object or undefined on error
 */
export function asn1Encode(encoding: EASN1TransportEncoding | undefined, argument: unknown, converter: IConverter, errors: ConverterErrors, encodeContext: EncodeContext): asn1ts.Sequence | object | undefined {
	switch (encoding) {
		case EASN1TransportEncoding.JSON:
			return converter.toJSON(argument, errors, encodeContext);
		case EASN1TransportEncoding.BER:
			return converter.toBER(argument, errors, encodeContext);
		case undefined:
		default:
			debugger;
	}
	return undefined;
}

/**
 * The base class for the ROSE handling. Contains code which is shared among the implementations
 * e.g. contains the handling method for an inbound request after we have sorted out which request it is (what is data object and the the handling method)
 */
export abstract class ROSEBase implements IASN1LogCallback {
	protected readonly transport: IASN1Transport;
	/**
	 * This property defines if we dispatch events right away or cache them in the cachedEvents
	 * It allows us to handle session setup before we receive events from the other side
	 */
	protected handleEvents: boolean;

	/** Array that contains events until we enabled processing them */
	protected cachedEvents: ICachedEvent[] = [];

	/**
	 * Constructs the ROSEBase class object
	 *
	 * @param transport - the transport object
	 * @param handleEvents - Whether events of this module shall get handle or not
	 */
	public constructor(transport: IASN1Transport, handleEvents: boolean) {
		this.transport = transport;
		this.handleEvents = handleEvents;
	}

	/**
	 * Starts event dispatching. Dispatches queued events first and then sets the flag to handle them directly.
	 */
	public dispatchEvents(): void {
		if (this.cachedEvents.length) {
			this.transport.log(ELogSeverity.debug, "Going to dispatch queued events", "dispatchEvents", this, { count: this.cachedEvents.length });
			let event = this.cachedEvents.shift();
			while (event) {
				event.invokeContext.handleEvent = true;
				void this.onInvoke(event.invoke, event.invokeContext, event.handler);
				event = this.cachedEvents.shift();
			}
		}
		this.handleEvents = true;
	}

	/**
	 * Empties the list of cached Events.
	 */
	public flushEvents(): void {
		this.cachedEvents = [];
	}

	/**
	 * Stops event handling (Starts caching events).
	 */
	public stopEvents(): void {
		this.handleEvents = false;
	}

	public abstract getLogData(): IASN1LogData;
	public abstract getNameForOperationID(id: number): string;
	public abstract onInvoke(invoke: ROSEInvoke, invokeContext: IReceiveInvokeContext, handler: unknown): Promise<ROSEReject | ROSEResult | ROSEError | undefined>;
	public abstract readonly logFilter: string[];

	/**
	 * Creates a ROSEError object based on an AsnRequestError object
	 *
	 * @param encoding - the encoding to use
	 * @param invoke - the invoke object
	 * @param error - the error to convert into a ROSEError
	 * @returns - the created ROSEError object
	 */
	private createROSEError(encoding: EASN1TransportEncoding | undefined, invoke: ROSEInvoke, error: ENetUC_Common.AsnRequestError): ROSEError | ROSEReject {
		return ROSEBase.createROSEError(this.transport, this, encoding, invoke, error);
	}

	/**
	 * Creates a ROSEError object based on an AsnRequestError object
	 *
	 * @param transport - the transport that handles the error (needed for logging and to get the proper encode context)
	 * @param logCallback - the callback to write a log entry in case we cannot encode the argument
	 * @param encoding - the encoding to use
	 * @param invoke - the invoke object
	 * @param error - the error to convert into a ROSEError
	 * @returns - the created ROSEError object
	 */
	public static createROSEError(transport: IASN1Transport, logCallback: IASN1LogCallback, encoding: EASN1TransportEncoding | undefined, invoke: ROSEInvoke, error: ENetUC_Common.AsnRequestError): ROSEError | ROSEReject {
		let result: ROSEError | ROSEReject;
		const converterErrors = new ConverterErrors();
		const encoded = asn1Encode(encoding, error, ENetUC_Common_Converter.AsnRequestError_Converter, converterErrors, transport.getEncodeContext());
		if (encoded) {
			result = new ROSEError({
				invokedID: invoke.invokeID,
				error_value: error.iErrorDetail,
				error: encoded
			});
		} else {
			const payLoad = ROSEBase.getDebugPayload(error);
			const diagnostic = converterErrors.getDiagnostic();
			transport.log(ELogSeverity.error, "Could not encode error", "createROSEError", logCallback, { encoding, payLoad, diagnostic });
			// If you land here, check the payLoad why it could not get encoded
			debugger;
			result = createInvokeReject(invoke, CustomInvokeProblemEnum.internalError, "Failed to encode ROSEError object");
		}
		return result;
	}

	/**
	 * Creates a ROSEError object based on an AsnRequestError object
	 *
	 * @param encoding - the encoding to use
	 * @param invoke - the invoke object
	 * @param invokeResult - the result we want to encode into the ROSEResult
	 * @param converter - the converter to encode the invokeResult
	 * @returns - the created ROSEError object
	 */
	private createROSEResult(encoding: EASN1TransportEncoding | undefined, invoke: ROSEInvoke, invokeResult: unknown, converter: IConverter): ROSEResult | ROSEReject {
		validateIsDedicatedObject(invokeResult);
		let result: ROSEResult | ROSEReject;
		const converterErrors = new ConverterErrors();
		const encoded = asn1Encode(encoding, invokeResult, converter, converterErrors, this.transport.getEncodeContext());
		if (encoded) {
			result = new ROSEResult({
				invokeID: invoke.invokeID,
				result: new ROSEResultSeq({
					resultValue: 0,
					result: encoded
				})
			});
		} else {
			const payLoad = ROSEBase.getDebugPayload(invokeResult as object);
			const diagnostic = converterErrors.getDiagnostic();
			this.transport.log(ELogSeverity.error, "Could not encode result", "createROSEResult", this, { encoding, payLoad, diagnostic });
			// If you land here, check the payLoad why it could not get encoded
			debugger;
			result = createInvokeReject(invoke, CustomInvokeProblemEnum.internalError, "Failed to encode ROSEResult object");
		}

		return result;
	}

	/**
	 * Encodes the argument for an invoke, the result is the object that contains the payload to send it to the other side
	 * First encodes the argument with the appropriate ArgumentConverter
	 * Afterwards the encoded argument is embedded in a ROSEInvoke object and encoded
	 *
	 * @param argument - the invoke argument
	 * @param operationID - the operation ID that has been called
	 * @param argumentConverter - the converter for the argument object into the different encodings
	 * @param event - true, in case we are encoding an event, false for a regular invoke
	 * @param context - the invokeContext which has already been prefilled with session related details
	 * @returns undefined on success or an AsnInvokeProblem on error
	 */
	private encodeInvoke(argument: unknown, operationID: number, argumentConverter: IConverter, event: boolean, context?: Partial<ISendInvokeContext>): AsnInvokeProblem | IASN1InvokeData {
		// The root ROSE message
		const message = new ROSEMessage();
		const sessionID = this.transport.getSessionID();
		const operationName = this.getNameForOperationID(operationID);
		const invokeID = event ? 99999 : this.transport.getNextInvokeID();
		message.invoke = new ROSEInvoke({
			invokeID,
			...(sessionID && { sessionID }),
			operationID,
			operationName
		});

		const encoding = context?.encoding || this.transport.getEncoding(context?.clientConnectionID);
		const invokeContext = new SendInvokeContext({
			...context,
			encoding,
			operationName,
			operationID,
			invokeID
		});

		// Copy the invoke for logging purposes (Contains plain json data)
		const invoke: ROSEInvoke = {
			...message.invoke,
			argument
		};

		const converterErrors = new ConverterErrors();
		const encodeContext = this.transport.getEncodeContext();
		{
			// Encode the embedded invoke argument
			const encoded = asn1Encode(encoding, argument, argumentConverter, converterErrors, encodeContext);
			if (converterErrors.length)
				return new AsnInvokeProblem(InvokeProblemenum.mistypedArgument, "Errors encoding ROSEInvoke.argument");
			else if (!message.invoke)
				return new AsnInvokeProblem(InvokeProblemenum.mistypedArgument, "ROSEInvoke.argument not set");
			message.invoke.argument = encoded;
		}

		let payLoad: Uint8Array | string;
		let logMessage: Uint8Array | ROSEMessage;
		{
			// Encode the ROSE envelop
			const encoded = asn1Encode(encoding, message, ROSEMessage_Converter, converterErrors, encodeContext);
			if (converterErrors.length)
				return new AsnInvokeProblem(InvokeProblemenum.mistypedArgument, "Errors encoding ROSEMessage");
			if (encoding === EASN1TransportEncoding.BER) {
				payLoad = new Uint8Array((encoded as asn1ts.Sequence).toBER());
				logMessage = payLoad;
			} else {
				payLoad = JSON.stringify(encoded, null, encodeContext.bPrettyPrint ? "\t" : undefined);
				logMessage = encoded as ROSEMessage;
			}
		}

		const result: IASN1InvokeData = {
			invoke,
			logMessage,
			payLoad,
			context: invokeContext
		};

		return result;
	}

	/**
	 * Handles an outbound event, encodes the argument, calls the other side and receives the response
	 *
	 * @param argument - the invoke argument
	 * @param operationID - the operation ID that has been called
	 * @param argumentConverter - the converter for the argument object into the different encodings
	 * @param invokeContext - the invokeContext which has already been prefilled with session related details
	 * @returns undefined or, if bSendEventSynchronous has been set true when the event was sent
	 */
	public handleEvent(argument: object, operationID: number, argumentConverter: IConverter, invokeContext?: ISendInvokeContextParams): undefined | boolean {
		// Encodes the argument and the ROSEInvoke envelop
		const result = this.encodeInvoke(argument, operationID, argumentConverter, true, invokeContext);
		if (result instanceof AsnInvokeProblem)
			return undefined;

		this.transport.logInvoke("handleEvent", this, argument, result.context, true);

		return this.transport.sendEvent(result);
	}

	/**
	 * Handles an outbound invoke, encodes the argument, calls the other side and receives the response
	 *
	 * @param argument - the invoke argument
	 * @param resultObj - the result object
	 * @param operationID - the operation ID that has been called
	 * @param argumentConverter - the converter for the argument object into the different encodings
	 * @param resultConverter - the converter for the result object into the different encodings
	 * @param invokeContext - the invokeContext which has already been prefilled with session related details
	 * @param errorConverter - the converter for the error object into the different encodings
	 * @returns the answer for the other side (either the result on succes or a ROSEError or ROSEReject
	 */
	public async handleInvoke<T, U = ENetUC_Common.AsnRequestError>(argument: object, resultObj: IASN1DataClass, operationID: number, argumentConverter: IConverter, resultConverter: IConverter, invokeContext?: ISendInvokeContextParams, errorConverter: IConverter = ENetUC_Common_Converter.AsnRequestError_Converter): Promise<T | U | AsnInvokeProblem> {
		const result = this.encodeInvoke(argument, operationID, argumentConverter, false, invokeContext);
		if (result instanceof AsnInvokeProblem)
			return result;

		const method = "invoke_" + result.context.operationName;
		const context = result.context;

		this.transport.logInvoke("handleInvoke", this, argument, context, true);

		const roseResult = await this.transport.sendInvoke(result);
		// The outer ROSE envelop has already been decoded here by the receive mesage in TSASN1Base!

		let payLoad: string | object | undefined;
		const converterErrors = new ConverterErrors();
		if (roseResult instanceof ROSEResult && roseResult.result) {
			const result = asn1Decode<T>(roseResult.result.result, resultConverter, converterErrors, this.transport.getDecodeContext(), context);
			if (result) {
				this.transport.logResult(method, this, result, context, false);
				return result;
			} else
				payLoad = ROSEBase.getDebugPayload(roseResult.result.result);
		} else if (roseResult instanceof ROSEError && roseResult.error) {
			const error = asn1Decode<U>(roseResult.error, errorConverter, converterErrors, this.transport.getDecodeContext(), context);
			if (error) {
				this.transport.logError(method, this, error, argument, context, false);
				return error;
			} else
				payLoad = ROSEBase.getDebugPayload(roseResult.error);
		} else if (roseResult instanceof ROSEReject) {
			const reject = asn1Decode<ROSEReject>(roseResult.reject, ROSEReject_Converter, converterErrors, this.transport.getDecodeContext(), context);
			if (reject) {
				this.transport.logReject(method, this, reject, argument, context, false);
				return handleRoseReject(reject);
			} else
				payLoad = ROSEBase.getDebugPayload(roseResult);
		} else if (roseResult)
			payLoad = ROSEBase.getDebugPayload(roseResult);

		const diagnostic = converterErrors.getDiagnostic();
		this.transport.log(ELogSeverity.error, "Could not decode invoke response", method, this, { encoding: context.encoding, payLoad, expected_type: resultObj.type, diagnostic });
		// If you land here, check the payLoad what the other side has replied to our request
		debugger;
		return new AsnInvokeProblem(InvokeProblemenum.mistypedArgument, diagnostic);
	}

	/**
	 * Handles an inbound event (invoke without result), decodes the argument, adds the encoding to the invokeContext, calls the handler
	 *
	 * @param invoke - the ROSE invoke object which contains the ROSE envelop as well as the request payload object
	 * @param operationID - the operation ID that has been called
	 * @param argumentClass - the data class which will be used to hold the invoke.argument after it has been decoded
	 * @param argumentConverter - the converter for the argument object into the different encodings
	 * @param handler - the handling class that holds the method (used to bind the method to the class instance and to call setLogContext)
	 * @param method - the handling method that receives the request if the argument has been decoded
	 * @param invokeContext - the invokeContext which has already been prefilled with session related details
	 * @returns the answer for the other side (either the result on succes or a ROSEError or ROSEReject
	 */
	public async handleOnEvent(invoke: ROSEInvoke, operationID: number, argumentClass: IASN1DataClass, argumentConverter: IConverter, handler: IASN1HandlerClass, method: IOnEventMethod | undefined, invokeContext: IReceiveInvokeContext): Promise<ROSEReject | undefined> {
		let result: ROSEReject;

		const converterErrors = new ConverterErrors();
		const operationName = this.getNameForOperationID(operationID);

		invokeContext.init(operationID, operationName);
		const methodName = "onInvoke_" + operationName;

		const argument = asn1Decode(invoke.argument, argumentConverter, converterErrors, this.transport.getDecodeContext(), invokeContext);
		if (argument) {
			this.transport.logInvoke(methodName, this, argument, invokeContext, false);
			if (method) {
				method = method.bind(handler);
				if (handler.setLogContext)
					handler.setLogContext(argument, invokeContext);
				method(argument, invokeContext);
				return undefined;
			} else
				result = createInvokeReject(invoke, InvokeProblemenum.unrecognisedOperation, `${operationName} is not implemented`);
		} else {
			const diagnostic = converterErrors.getDiagnostic();
			const payLoad = ROSEBase.getDebugPayload(invoke.argument);
			this.transport.log(ELogSeverity.error, "Could not decode OnEvent argument", methodName, this, { encoding: invokeContext.encoding, payLoad, expected_type: argumentClass.type, diagnostic });
			// If you land here, check the payLoad what the other side has sent to us
			debugger;
			result = createInvokeReject(invoke, InvokeProblemenum.mistypedArgument, diagnostic);
		}

		this.transport.logReject(methodName, this, result, invoke, invokeContext, false);

		return result;
	}

	/**
	 * Handles an inbound invoke, decodes the argument, adds the encoding to the invokeContext, calls the handler and handles the result
	 *
	 * @param invoke - the ROSE invoke object which contains the ROSE envelop as well as the request payload object
	 * @param operationID - the operation ID that has been called
	 * @param argumentClass - the data class which will be used to hold the invoke.argument after it has been decoded
	 * @param argumentConverter - the converter for the argument object into the different encodings
	 * @param resultConverter - the converter for the result object into the different encodings
	 * @param handler - the handling class that holds the method (used to bind the method to the class instance and to call setLogContext)
	 * @param method - the handling method that receives the request if the argument has been decoded
	 * @param invokeContext - the invokeContext which has already been prefilled with session related details
	 * @returns the answer for the other side (either the result on succes or a ROSEError or ROSEReject
	 */
	public async handleOnInvoke(invoke: ROSEInvoke, operationID: number, argumentClass: IASN1DataClass, argumentConverter: IConverter, resultConverter: IConverter, handler: IASN1HandlerClass, method: IOnInvokeMethod | undefined, invokeContext: IReceiveInvokeContext): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		let result: ROSEReject | ROSEResult | ROSEError | undefined;

		const converterErrors = new ConverterErrors();
		const operationName = this.getNameForOperationID(operationID);

		invokeContext.init(operationID, operationName);
		const methodName = "onInvoke_" + operationName;

		let handlerResult: object | undefined;
		const argument = asn1Decode(invoke.argument, argumentConverter, converterErrors, this.transport.getDecodeContext(), invokeContext);
		if (argument) {
			this.transport.logInvoke(methodName, this, argument, invokeContext, false);
			if (method) {
				method = method.bind(handler);
				if (handler.setLogContext)
					handler.setLogContext(argument, invokeContext);
				handlerResult = await method(argument, invokeContext);
			}
			if (handlerResult instanceof ENetUC_Common.AsnRequestError)
				result = this.createROSEError(invokeContext.encoding, invoke, handlerResult);
			else if (handlerResult)
				result = this.createROSEResult(invokeContext.encoding, invoke, handlerResult, resultConverter);
			else
				result = createInvokeReject(invoke, InvokeProblemenum.unrecognisedOperation, `${operationName} is not implemented`);
		} else {
			const diagnostic = converterErrors.getDiagnostic();
			const payLoad = ROSEBase.getDebugPayload(invoke.argument);
			this.transport.log(ELogSeverity.error, "Could not decode OnInvoke argument", methodName, this, { encoding: invokeContext.encoding, payLoad, expected_type: argumentClass.type, diagnostic });
			// If you land here, check the payLoad what the other side has replied to our request
			debugger;
			result = createInvokeReject(invoke, InvokeProblemenum.mistypedArgument, diagnostic);
		}

		if (result instanceof ROSEResult && handlerResult)
			this.transport.logResult(methodName, this, handlerResult, invokeContext, true);
		else if (result instanceof ROSEError && handlerResult)
			this.transport.logError(methodName, this, handlerResult, argument, invokeContext, true);
		else if (result instanceof ROSEReject)
			this.transport.logReject(methodName, this, result, argument, invokeContext, true);
		else
			debugger;

		return result;
	}

	/**
	 * Converts data into the appropriate log message notation (e.g. BER as hex string)
	 *
	 * @param data - the data to convert
	 * @returns - the log message payload
	 */
	public static getDebugPayload(data: object | Uint8Array | asn1ts.Sequence): string | object {
		if (data instanceof Uint8Array)
			return ROSEBase.buf2hex(data);
		else if (data instanceof asn1ts.Sequence)
			return ROSEBase.buf2hex(data.valueBeforeDecodeView);
		else
			return data;
	}

	/**
	 * Converts an array buffer to hex notation
	 *
	 * @param buffer - the buffer to convert
	 * @param separator - an optional delimieter between the hex encoded bytes (defaults to no additional spacing)
	 * @returns the buffer in hex string notation
	 */
	public static buf2hex(buffer: ArrayBuffer | Uint8Array, separator = ""): string {
		if (buffer instanceof ArrayBuffer) {
			return [...new Uint8Array(buffer)]
				.map(x => x.toString(16).padStart(2, "0"))
				.join(separator);
		} else {
			const data = new Array<string>();
			for (const elem of buffer)
				data.push(elem.toString(16).padStart(2, "0"));
			return data.join(separator);
		}
	}

	/**
	 * Converts string data which is stored in an Uint8Array back into the string notation
	 *
	 * @param bytes - the data to convert
	 * @returns the string
	 */
	public static bytesToSring(bytes: Uint8Array): string {
		return String.fromCharCode(...bytes);
	}
}
