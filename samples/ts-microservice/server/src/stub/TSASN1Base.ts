// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do NOT edit or modify this code as it is machine generated
// and will be overwritten with every code generation of the esnacc.exe

// prettier-ignore
/* eslint-disable */

import * as asn1ts from "@estos/asn1ts";

import * as ENetUC_Common from "./ENetUC_Common";
import { InvokeProblemenum, ROSEError, ROSEInvoke, ROSEMessage, ROSEReject, ROSEResult } from "./SNACCROSE";
import { ROSEMessage_Converter } from "./SNACCROSE_Converter";
import { DecodeContext, ConverterErrors, EncodeContext, IEncodeContext } from "./TSConverterBase";
import { createInvokeReject, ELogSeverity, IReceiveInvokeContext, ISendInvokeContext, IInvokeHandler, IROSELogger, IASN1LogData, IASN1LogCallback, CustomInvokeProblemEnum, IInvokeContextBase, EASN1TransportEncoding, asn1Decode, IASN1Transport, IASN1InvokeData, asn1Encode, ROSEBase, ReceiveInvokeContext } from "./TSROSEBase";

// Original part of uclogger, duplicated here as we use it in frontend and backend the same
interface ILogData {
	className: string;
	classProps?: Record<string, unknown>;
}

// The type of instance the TASN1 belongs to (helpfull for debugging if two instance are running in parallel (e.g. ASN1Server and ASN1NodeClient)
export enum ASN1ClassInstanceType {
	TSASN1Server = 0,
	TSASN1BrowserClient = 1,
	TSASN1NodeClient = 2
}

/**
 * A handler object covers where to pass a rose invoke after it has been decoded
 * Every handler targets one function based on the operationID.
 * The handlers are stored in a Map (below) to ease up finding the entrie for a called function
 * The handler holds the entry for the switch case (of the module) that actually decodes the request argument and handles result and errors
 * as well as the interface handling the request (with the parsed argument) in the end.
 */
class Handler {
	// This handler implements the oninvokemethod with the embedded switch case for all the methods within the module
	public readonly oninvokehandler: IInvokeHandler;
	// The operationID as specified in the ASN.1 Description
	public readonly operationID: number;
	// The operation name as specified in the ASN.1 Description
	public readonly operationName: string;
	// This is the object/handler which actually implements the called functionality
	public readonly requesthandler: never;

	/**
	 * Constructs a handler object, simply stores the handed over arguments in the class
	 *
	 * @param oninvokehandler - The handler that will handle operationID / operationName
	 * @param requesthandler - The requesthandler that will handle the request as decoded in oninvokehandler
	 * @param operationID - The id we registger
	 * @param operationName - The name we registger
	 */
	public constructor(oninvokehandler: IInvokeHandler, requesthandler: never, operationID: number, operationName: string) {
		this.oninvokehandler = oninvokehandler;
		this.requesthandler = requesthandler;
		this.operationID = operationID;
		this.operationName = operationName;
	}
}

// The base class for all log entries
export interface IROSELogEntryBase {
	_type?: "IInvokeLogEntry" | "IResultLogEntry" | "IErrorLogEntry" | "IRejectLogEntry";
	operationName: string;
	context?: IReceiveInvokeContext | ISendInvokeContext;
	isOutbound: boolean;
	transport: true;
	invokeID: number;
}

// This is a invoke LogEntry. It is handed over as meta data to the logger in the transport layer
export interface IInvokeLogEntry extends IROSELogEntryBase {
	_type?: "IInvokeLogEntry";
	isEvent: boolean;
	argument: object;
	argumentName: string;
}

// This is a result LogEntry. It is handed over as meta data to the logger in the transport layer
export interface IResultLogEntry extends IROSELogEntryBase {
	_type?: "IResultLogEntry";
	result: object;
	resultName: string;
}

// This is an error LogEntry. It is handed over as meta data to the logger in the transport layer
export interface IErrorLogEntry extends IROSELogEntryBase {
	_type?: "IErrorLogEntry";
	invokeArgument?: object;
	invokeArgumentName?: string;
	error: object;
}

// This is a reject LogEntry. It is handed over as meta data to the logger in the transport layer
export interface IRejectLogEntry extends IROSELogEntryBase {
	_type?: "IRejectLogEntry";
	invokeArgument?: object;
	invokeArgumentName?: string;
	reject: object;
}

/**
 * Encapsulates the json encoded data to send with an optional result value we want to return
 */
export interface IResponseData {
	// JSON encoded payload to put into the body
	payLoad: Uint8Array | string;
	// HTTP result value for the client
	httpStatusCode: number;
}

/**
 * For each invoke towards the other side we create a completion object called PendingInvoke
 * Each invoke gets a dedicated invoke id assigned
 * This invoke id is also part of any result in association with the request.
 * thus we can store the promise completion methods the invoke provided and return the asynchronous response with this PendingInvoke object
 * if the server does not answer we have a timer which automatically completes after a certain amount of time (argument of the invoke)
 */
export class PendingInvoke {
	// The callback pointer the invoker provided
	private readonly resolve: (value?: ROSEReject | ROSEResult | ROSEError) => void;
	// private readonly reject: (reason?: never) => void;

	// The timerid (if used) for the settimeout, if the server does not answer in time the object is completed automatically with a reject message
	private timerID?: ReturnType<typeof setTimeout> = undefined;
	// the original Invoke to be able to provide the timeout reject message
	private readonly invoke: ROSEInvoke;
	// the context of the invoke
	private readonly context: ISendInvokeContext;
	// the encoding that has been used to send the data
	private readonly encoding: IEncodeContext;

	/**
	 * Constructs a pending invoke object, simply stores the handed over arguments in the class
	 *
	 * @param invoke - The original invoke message where we are waiting for a response
	 * @param context - The context that contains details about the invoke
	 * @param encoding - The encoding beeing used to send the invoke (needed to properly encode the reject in case it´s needed)
	 * @param resolve - The resolve method we call in case we receive a proper ROSE Response (Reject, Result, Error)
	 * @param reject - The reject method - (currently not used)
	 * @param timerID - The timerID of the timer that has been created to handle the timeout
	 * The timer is created outside as it depends on if we are running in a browser or in node
	 */
	public constructor(invoke: ROSEInvoke, context: ISendInvokeContext, encoding: IEncodeContext, resolve: (value?: ROSEReject | ROSEResult | ROSEError) => void, reject: (reason?: unknown) => void, timerID?: ReturnType<typeof setTimeout>) {
		this.invoke = invoke;
		this.context = context;
		this.encoding = encoding;
		this.resolve = resolve;
		// this.reject = reject;
		this.timerID = timerID;
	}

	/**
	 * Called from the TSASN1Base when the result object was received and decoded
	 *
	 * @param result - The rose result object
	 */
	public completed_result(result: ROSEResult): void {
		this.clearTimeout();
		this.resolve(result);
	}

	/**
	 * Called from the TSASN1Base when the error object was received and decoded
	 *
	 * @param error - The rose error object
	 */
	public completed_error(error: ROSEError): void {
		this.clearTimeout();
		this.resolve(error);
	}

	/**
	 * Called from the TSASN1Base when the reject object was received and decoded
	 *
	 * @param reject - The rose reject object
	 */
	public completed_reject(reject: ROSEReject): void {
		this.clearTimeout();
		this.resolve(reject);
	}

	/**
	 * Called from the TSASN1Base if a timeout occured and the regular answer has not jet been provided
	 */
	public complete_timedout(): void {
		if (this.timerID) {
			this.clearTimeout();
			const reject = new ROSEReject({
				invokedID: {
					invokedID: this.invoke.invokeID
				},
				sessionID: this.invoke.sessionID,
				details: "timed out",
				reject: {
					invokeProblem: CustomInvokeProblemEnum.requestTimedOut
				}
			});

			// We need to encode it in the encoding the client is currently awaiting from the other side
			// IF the encoding is BER we need to encoe the element in BER 

			this.resolve(reject);
		}
	}

	/**
	 * clear the timer in case we did receive a proper response for this pending operation
	 */
	private clearTimeout(): void {
		if (this.timerID) {
			clearTimeout(this.timerID);
			this.timerID = undefined;
		}
	}
}

/**
 * List of pending invokes
 */
class PendingInvokes extends Map<number, PendingInvoke> {
}

/**
 * A helper object that contains the log data
 */
export interface ITransportMetaData {
	clientConnectionID: string | undefined;
	clientIP: string | undefined;
	invokeID: number | undefined;
	direction: "in" | "out";
	type: "ws" | "rest";
	operationID: number | undefined;
	operationName: string | undefined;
	payLoad: string | object;
}

/**
 * Result of the getCallingFunction
 */
export interface IASN1CallStackEntry {
	class?: string;
	method: string | undefined;
}

/**
 * Base class for the TASN1Server and the node and browser Client
 */
export abstract class TSASN1Base implements IASN1Transport {
	// Invoke counter which is incremented with every invoke. Counts up to 99998 and then returns to 0. Invokes with counter value 99999 are events which implicates not to send a result
	private invokeCounter = 0;
	// Holds the parameters for the toJSON encoding
	protected encodeContext = new EncodeContext({ bPrettyPrint: false, bAddTypes: true, bUCServerOptionalParams: false });
	// Holds the parameters for the fromJSON decoding
	protected decodeContext = new DecodeContext(false);
	// List of pending invokes (as described above)
	protected pendingInvokes = new PendingInvokes();
	// The encoding to use for outbound calls (may be overwritten by custom settings in the invokeContext)
	protected encoding: EASN1TransportEncoding;
	// Holds all registered invoke handlers
	private handlersByID = new Map<number, Handler>();
	private handlersByName = new Map<string, Handler>();
	// The Logger Callback which must be set with the SetLogger Method
	protected logger?: IROSELogger;
	// Logs the raw transport (inbound before decoding, outbound after encoding)
	protected logRawTransport = false;
	// The default timeout for a websocket based invoke to the other side (a caller may change that value or provide a different one in the per call invokecontext)
	protected defaultTimeout = 5000;
	// Addtitional meta data that is added to general log requests
	protected additionalLogMeta: Record<string, unknown> | undefined;
	// Addtitional meta data that is added to raw log requests (e.g. if you want to send data to loki)
	protected additionalLogMetaForRawTransport: Record<string, unknown> | undefined;
	// The type of instance of this class (Helps debugging if two instances are running in parallel (TSASN1NodeClient and TSASN1Server)
	protected readonly instanceType: ASN1ClassInstanceType;

	/**
	 * Constructs the ASN1 base class
	 *
	 * @param encoding - sets the encoding for outbound calls
	 * @param instanceType - the type of instance this class belongs to
	 */
	public constructor(encoding: EASN1TransportEncoding.JSON | EASN1TransportEncoding.BER, instanceType: ASN1ClassInstanceType) {
		this.instanceType = instanceType;
		this.encoding = encoding;
	}

	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 *
	 * @returns - an IASN1LogData log data object provided additional data for all the logger calls in this class
	 */
	public getLogData(): IASN1LogData {
		return { className: this.constructor.name };
	}

	/**
	 * Gets the encoding from the transport
	 * This method is overwritten in the server to get the encoding of a certain client connection
	 *
	 * @param clientConnectionID - the clientConnectionID for which we want to get the encoding
	 * @returns - the encoding for a certain clientConnectionID or the default encoding of this instance
	 */
	public getEncoding(clientConnectionID?: string): EASN1TransportEncoding {
		return this.encoding;
	}

	/**
	 * Sets the encoding for the transport
	 *
	 * @param encoding - the encoding to set
	 */
	public setEncoding(encoding: EASN1TransportEncoding): void {
		this.encoding = encoding;
	}

	/**
	 * Method to register an invoke handler (is called from the constructors in the generated ROSE classes)
	 *
	 * @param oninvokehandler - this is the handler that will receive the invoke for operationID / operationName
	 * in the invokehandler we have this switch case which contains all the decoders for the arguments of the functinos
	 * if the request has been succesfully decoded, the request handler is called
	 * -
	 * Basically the invokeHandler takes care of the rose decoding argument, encoding result where the requesthandler deals
	 * with the plain argument and result objects
	 * @param requesthandler - this is the class that is implementing the function (operationID / operationName)
	 * @param operationID - the id of the operation that registers
	 * @param operationName - the name of the operation that registers
	 */
	public registerOperation(oninvokehandler: IInvokeHandler, requesthandler: never, operationID: number, operationName: string): void {
		if (!this.handlersByID.has(operationID)) {
			const handler = new Handler(oninvokehandler, requesthandler, operationID, operationName);
			this.handlersByID.set(operationID, handler);
			this.handlersByName.set(operationName, handler);
		}
	}

	/**
	 * The client is requested to set a logger interface that will take care of processing log entries the stub is creating
	 * The stub is trying to add log messages whereever possible. So setting the logger will not only provide errors and warning messages
	 * but also debug messages for invokes, results, rejects etc.
	 *
	 * @param logger - a class/object that has to fullfill the IROSELogger interface to receive the log messages of the stub
	 * @param logRawTransport - wether to log the raw transport date (logged before decoding and after encoding)
	 * @param additionalMeta - data that is added to the meta data for a general log requets
	 * @param rawAdditionalMeta - data that is added to the raw meta data for a general log requets
	 */
	public setLogger(logger: IROSELogger | undefined, logRawTransport = false, additionalMeta?: Record<string, unknown>, rawAdditionalMeta?: Record<string, unknown>): void {
		this.logger = logger;
		this.logRawTransport = logRawTransport;
		this.additionalLogMeta = additionalMeta;
		this.additionalLogMetaForRawTransport = rawAdditionalMeta;
	}

	/**
	 * A development debug log that shows the instance type with the log message
	 *
	 * @param log - the log text to print
	 * @param optionalParams - additional parameters for the variable argument list
	 */
	public debugLog(log: string, ...optionalParams: any[]): void {
		(console as Console).log(this.getASN1ClassInstanceType(), log, ...optionalParams);
	}

	/**
	 * The central log method for the whole stub
	 *
	 * @param severity - severity of the log entry (error, info, warn, debug)
	 * @param message - The message for the log entry, do NOT add contextual data into this message, use the meta data for it
	 * @param calling_method - name of the caller
	 * @param cbOrLogData - The callback fills the classname as well as contextual data you want to have in every log entry (e.g. sessionids)
	 * @param meta - Meta data you want to have logged with the message (arguments, results, intermediate data, anything that might be usefull later)
	 * @param exception - In case of an exception pass it here.
	 */
	public log(severity: ELogSeverity, message: string, calling_method: string, cbOrLogData: IASN1LogCallback | ILogData, meta?: unknown, exception?: unknown): void {
		if (this.logger) {
			// If additional log data has been added, merge it now into the meta data
			if (this.additionalLogMeta) {
				if (meta) {
					meta = {
						...(meta as object),
						...this.additionalLogMeta
					};
				} else
					meta = this.additionalLogMeta;
			}

			let logData: ILogData;
			if (typeof (cbOrLogData as IASN1LogCallback).getLogData === "function")
				logData = (cbOrLogData as IASN1LogCallback).getLogData();
			else
				logData = cbOrLogData as ILogData;

			switch (severity) {
				case ELogSeverity.error:
					this.logger.error(message, calling_method, logData, meta, exception);
					break;
				case ELogSeverity.warn:
					this.logger.warn(message, calling_method, logData, meta, exception);
					break;
				case ELogSeverity.info:
					this.logger.info(message, calling_method, logData, meta, exception);
					break;
				case ELogSeverity.debug:
					this.logger.debug(message, calling_method, logData, meta, exception);
					break;
				default:
					throw new Error("unhandled loglevel");
			}
		}
	}

	/**
	 * Helper method that allows to send an event without setting a timeout or by mistake taking care of result messages of the sendInvoke message
	 *
	 * @param data - the data for the invoke
	 * @returns undefined or, if bSendEventSynchronous has been set true when the event was sent
	 */
	public sendEvent(data: IASN1InvokeData): undefined | boolean {
		if (data.invokeContext?.bSendEventSynchronous)
			return this.sendEventSync(data);
		else {
			void this.sendInvoke(data);
			return undefined;
		}
	}

	// Sends an invoke (event, invoke, reject, result etc.) to the other side
	public abstract sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined>;
	// Sends an even synchronously to the other side of a websocket connection
	public abstract sendEventSync(data: IASN1InvokeData): boolean;
	// Retrieves the client connection ID we got from the server
	public abstract getSessionID(): string | undefined;

	/**
	 * The client and server implementation needs to pass received data (the transport blob) into this function
	 * based on the result of the internal function call the method creates the answer which will be sent back to the caller.
	 *
	 * @param rawData - the raw data as read from the transport
	 * @param invokeContext - Invoke context as specified from the caller (if we have something like a client connection handler, we will have a sessionid. At least some identifier of the client should be provided (e.g. the ip address)
	 * @returns - the return data and http status code
	 */
	public async receive(rawData: object | Uint8Array, invokeContext: ReceiveInvokeContext): Promise<IResponseData | void> {
		const errors = new ConverterErrors();
		// Decodes the outer ROSE envelop, the embedded elements are not decoded:
		// ROSEInvoke.argument
		// ROSEResult.result.result
		// ROSEError.error
		const message = asn1Decode<ROSEMessage>(rawData, ROSEMessage_Converter, errors, this.getDecodeContext(), invokeContext);
		if (message)
			return this.receiveHandleROSEMessage(message, rawData, invokeContext);

		const payLoad = ROSEBase.getDebugPayload(rawData);
		this.log(ELogSeverity.error, "Failed to decode ROSEMessage", "receive", this, { payLoad, errors });
		const result = createInvokeReject(0, InvokeProblemenum.unexpectedChildOperation, "Failed to parse argument " + errors.getDiagnostic());
		// If the request was not parsed and the encoding has not been defined we now need to specify one for the result
		if (!invokeContext.encoding)
			invokeContext.encoding = EASN1TransportEncoding.JSON;
		return this.handleResult(result, invokeContext);
	}

	/**
	 * Handles the decoded ROSEMessage from the receive method
	 *
	 * @param message - the decoded ROSEMessage
	 * @param rawData - the rawData (for logging)
	 * @param invokeContext - the invokeContext
	 * @returns - the return data and http status code
	 */
	public async receiveHandleROSEMessage(message: ROSEMessage, rawData: object | Uint8Array, invokeContext: ReceiveInvokeContext): Promise<IResponseData | void> {
		let result: ROSEReject | ROSEResult | ROSEError | undefined;
		try {
			if (message.invoke) {
				invokeContext.invokeID = message.invoke.invokeID;
				invokeContext.operationID = message.invoke.operationID;
				if (message.invoke.operationName)
					invokeContext.operationName = message.invoke.operationName;
				else {
					// In case the client did not provide an operationName, look it up
					// This only works if we have a registered handler for the operation
					const handler = this.getHandler(invokeContext.operationID);
					if (handler)
						invokeContext.operationName = handler.operationName;
				}
				if (!invokeContext.clientConnectionID)
					invokeContext.clientConnectionID = message.invoke.sessionID;
			} else if (message.result)
				invokeContext.invokeID = message.result.invokeID;
			else if (message.error)
				invokeContext.invokeID = message.error.invokedID;
			else if (message.reject && message.reject.invokedID.invokedID)
				invokeContext.invokeID = message.reject.invokedID.invokedID;

			if (invokeContext.encoding === EASN1TransportEncoding.JSON)
				this.logTransport(message, "receive", "in", invokeContext);
			else
				this.logTransport(rawData, "receive", "in", invokeContext);

			if (message.invoke)
				result = await this.onROSEInvoke(message.invoke, invokeContext);
			else if (message.result)
				result = await this.onROSEResult(message.result, invokeContext);
			else if (message.error)
				result = await this.onROSEError(message.error, invokeContext);
			else if (message.reject)
				result = await this.onROSEReject(message.reject, invokeContext);
			else
				result = createInvokeReject(0, InvokeProblemenum.mistypedArgument, "No member was filled within the ROSEMessage");
		} catch (error) {
			this.log(ELogSeverity.error, "exception", "receive", this, undefined, error);
			result = createInvokeReject(0, InvokeProblemenum.unexpectedChildOperation, "Exception catched while trying to parse argument...");
		}
		return this.handleResult(result, invokeContext);
	}

	/**
	 * Logs the transport data
	 *
	 * @param result - the result to handle
	 * @param invokeContext - the invokeContext
	 * @returns - the responseData object
	 */
	protected handleResult(result: ROSEReject | ROSEResult | ROSEError | undefined, invokeContext: IReceiveInvokeContext): IResponseData | void {
		if (!result)
			return;

		let encoded: object | asn1ts.Sequence | undefined = undefined;
		const errors = new ConverterErrors();
		const encodeContext = this.getEncodeContext();

		const message = new ROSEMessage();
		let resultValue = 0;
		if (result instanceof ROSEReject) {
			if (result.reject?.generalProblem)
				resultValue = result.reject?.generalProblem;
			else if (result.reject?.invokeProblem)
				resultValue = result.reject?.invokeProblem;
			else if (result.reject?.returnErrorProblem)
				resultValue = result.reject?.returnErrorProblem;
			else if (result.reject?.returnResultProblem)
				resultValue = result.reject?.returnResultProblem;
			message.reject = result;
		} else if (result instanceof ROSEResult)
			message.result = result;
		else if (result instanceof ROSEError) {
			message.error = result;
			message.error.sessionID = invokeContext.clientConnectionID;
			resultValue = result.error_value;
		}

		if (resultValue === 0)
			resultValue = 200;
		else if (resultValue <= 200 || resultValue >= 600)
			resultValue = 500;
	
		// Encode the ROSE message with the embedded result object
		encoded = asn1Encode(invokeContext.encoding, message, ROSEMessage_Converter, errors, encodeContext);
		if (!encoded) {
			this.log(ELogSeverity.error, "Failed to encode result message", "receive", this, { errors: errors.getDiagnostic() });
			return;
		}

		let payLoad: Uint8Array | string;
		if (invokeContext.encoding === EASN1TransportEncoding.BER) {
			payLoad = new Uint8Array((encoded as asn1ts.Sequence).toBER());
			this.logTransport(payLoad, "receive", "out", invokeContext);
		} else {
			payLoad = JSON.stringify(encoded, null, encodeContext.bPrettyPrint ? "\t" : undefined);
			this.logTransport(encoded, "receive", "out", invokeContext);
		}

		return {
			payLoad,
			httpStatusCode: resultValue
		};
	}

	/**
	 * Logs the transport data
	 *
	 * @param payLoad - the payload to log
	 * @param method - the method which is calling the logTransport
	 * @param direction - the direction of this transport log entry
	 * @param invokeContext - the context of the invoke (connecitionID, clientID etc.)
	 */
	protected logTransport(payLoad: Uint8Array | string | object, method: string, direction: "in" | "out", invokeContext: IReceiveInvokeContext): void {
		if (this.logRawTransport) {
			if (payLoad instanceof Uint8Array) {
				if (invokeContext.encoding === EASN1TransportEncoding.JSON)
					payLoad = ROSEBase.bytesToSring(payLoad);
				else
					payLoad = ROSEBase.buf2hex(payLoad);
			}

			// Let´s log what we did receive
			const meta: ITransportMetaData = {
				...this.additionalLogMetaForRawTransport,
				clientConnectionID: invokeContext.clientConnectionID,
				clientIP: invokeContext.clientIP,
				invokeID: invokeContext.invokeID,
				type: invokeContext.clientConnectionID ? "ws" : "rest",
				operationID: invokeContext.operationID,
				operationName: invokeContext.operationName,
				direction,
				payLoad
			};
			this.log(ELogSeverity.debug, "asn1Transport", method, this, meta);
		}
	}

	/**
	 * Getter for the encoding parameters
	 *
	 * @returns - the EncodeContext telling the encoder how to encoder the transport data
	 */
	public getEncodeContext(): EncodeContext {
		return new EncodeContext(this.encodeContext);
	}

	/**
	 * Getter for the decoding parameters
	 *
	 * @returns - the DecodeContext telling the decoder how to decode the transport data
	 */
	public getDecodeContext(): DecodeContext {
		return new DecodeContext(this.decodeContext.bLaxDecoding);
	}

	/**
	 * Retrieves the next invokeID
	 *
	 * @returns - the incremented invokeID to be used for the next invoke
	 */
	public getNextInvokeID(): number {
		this.invokeCounter++;
		if (this.invokeCounter > 99998)
			this.invokeCounter = 0;
		return this.invokeCounter;
	}

	/**
	 * This logger is called when we decoded an invoke and are about to call the handler
	 *
	 * @param calling_method - the handler that is called next
	 * @param callback - the callback that provides additional contextual data
	 * @param argument - the payload (content of the ROSE invoke argument)
	 * @param context - the context that is passed along with an invoke
	 * @param isOutbound - true if this message goes outbound
	 */
	public logInvoke(calling_method: string, callback: IASN1LogCallback, argument: object, context: IReceiveInvokeContext | ISendInvokeContext, isOutbound: boolean): void {
		const meta: IInvokeLogEntry = {
			_type: "IInvokeLogEntry",
			operationName: context.operationName || "",
			invokeID: context.invokeID,
			context,
			isOutbound,
			isEvent: context.invokeID === 99999,
			transport: true,
			argument,
			argumentName: argument.constructor.name
		};

		const logData = {
			...callback.getLogData(),
			clientConnectionID: context.clientConnectionID,
			invokeID: context.invokeID
		};

		this.log(ELogSeverity.debug, `${meta.isEvent ? "event" : "invoke"} ${isOutbound ? "out" : "in"}`, calling_method, logData, meta);
	}

	/**
	 * This logger is called when we have a result from a handler method we have been calling
	 *
	 * @param calling_method - the handler that was called and provided us the result
	 * @param callback - the callback that provides additional contextual data
	 * @param result - the payload (content of the ROSE Result argument)
	 * @param context - the context that is passed along with an invoke
	 * @param isOutbound - true if this message goes outbound
	 */
	public logResult(calling_method: string, callback: IASN1LogCallback, result: object, context: IReceiveInvokeContext | ISendInvokeContext, isOutbound: boolean): void {
		const meta: IResultLogEntry = {
			_type: "IResultLogEntry",
			operationName: context.operationName,
			invokeID: context.invokeID,
			context,
			isOutbound,
			transport: true,
			result,
			resultName: result.constructor.name
		};

		const logData = {
			...callback.getLogData(),
			clientConnectionID: context.clientConnectionID,
			invokeID: context.invokeID
		};

		this.log(ELogSeverity.debug, `result ${isOutbound ? "out" : "in"}`, calling_method, logData, meta);
	}

	/**
	 * This logger is called when we have an error from a handler method we have been calling
	 *
	 * @param calling_method - the handler that was called and provided us the result
	 * @param callback - the callback that provides additional contextual data
	 * @param error - the rose error object
	 * @param invokeArgument - the invoke that was leading to the error repsonse
	 * @param context - the context of the invoke that lead to the reject
	 * @param isOutbound - true if this message goes outbound
	 */
	public logError(calling_method: string, callback: IASN1LogCallback, error: object, invokeArgument: object | undefined, context: IInvokeContextBase, isOutbound: boolean): void {
		const meta: IErrorLogEntry = {
			_type: "IErrorLogEntry",
			operationName: context.operationName,
			isOutbound,
			transport: true,
			invokeID: context.invokeID,
			invokeArgument,
			invokeArgumentName: invokeArgument?.constructor.name,
			error
		};

		const logData = {
			...callback.getLogData(),
			clientConnectionID: context.clientConnectionID,
			invokeID: context.invokeID
		};

		this.log(ELogSeverity.warn, `error ${isOutbound ? "out" : "in"}`, calling_method, logData, meta);
	}

	/**
	 * This logger is called when we did send an invoke and received a reject
	 *
	 * @param calling_method - the method that was called (some invoke we called)
	 * @param callback - the callback that provides additional contextual data
	 * @param reject - the reject that has been decoded
	 * @param invokeArgument - the invoke that was leading to the reject repsonse
	 * @param context - the context of the invoke that lead to the reject
	 * @param isOutbound - true if this message goes outbound
	 */
	public logReject(calling_method: string, callback: IASN1LogCallback, reject: object, invokeArgument: object | undefined, context: IInvokeContextBase, isOutbound: boolean): void {
		const meta: IRejectLogEntry = {
			_type: "IRejectLogEntry",
			operationName: context.operationName,
			isOutbound,
			transport: true,
			invokeID: context.invokeID,
			invokeArgument,
			invokeArgumentName: invokeArgument?.constructor.name,
			reject
		};

		const logData = {
			...callback.getLogData(),
			clientConnectionID: context.clientConnectionID,
			invokeID: context.invokeID
		};

		this.log(ELogSeverity.warn, "rejecting invoke", calling_method, logData, meta);
	}

	/**
	 * Searches for a handler in the registered operations (see registerOperation)
	 *
	 * @param operationID - the operationID for which we want to find a handler
	 * @returns - a handler if we found one or undefined
	 */
	protected getHandler(operationID: number): Handler | undefined {
		return this.handlersByID.get(operationID);
	}

	/**
	 * The ROSE invoke is handled here - If we find a handler we call the handler.
	 * If no handler is found a generic error message is returned
	 *
	 * @param invoke - The invoke message (invoke part is already decoded thus we can handle the invoke and decode the argument according to the function beeing called).
	 * (see onInvoke in the appropriate handler)
	 * @param invokeContext - The invoke related contextual data (see IReceiveInvokeContext)
	 * @returns - Returns one of the possible ROSE messages or undefined if the invoke was an event which has no appropriate result
	 */
	protected async onROSEInvoke(invoke: ROSEInvoke, invokeContext: IReceiveInvokeContext): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		try {
			const handler = this.getHandler(invoke.operationID);
			if (handler === undefined)
				return createInvokeReject(invoke, InvokeProblemenum.unrecognisedOperation, `There is no registered handler for operation ${invoke.operationName} (${invoke.operationID})`);
			else
				return await handler.oninvokehandler.onInvoke(invoke, invokeContext, handler.requesthandler);
		} catch (error) {
			// We want the debugger to catch that behaviour instantly -> so the developer can fix it right away
			// !!! An exception should NEVER EVER reach this point !!!
			// Handle all exceptions properly inside the oninvoke methods.
			debugger;
			if (error instanceof ENetUC_Common.AsnRequestError) {
				this.log(ELogSeverity.error, "LAST possible treatment of an unhandled exception. This exception MUST be handled inside the called function, not here!", "onROSEInvoke", this, invoke, error);
				return ROSEBase.createROSEError(this, this, invokeContext.encoding, invoke, error);
			} else {
				this.log(ELogSeverity.error, "LAST possible treatment of an unhandled exception. This exception MUST be handled inside the called function, not here!", "onROSEInvoke", this, invoke, error);
				return createInvokeReject(invoke, InvokeProblemenum.unexpectedChildOperation, "Exception catched while handling OnROSEInvoke");
			}
		}
	}

	/**
	 * The ROSE result is handled here - (we called the other side and received a result)
	 * Based on the invoke ID we search for a pending invoke in the list of pending invokes
	 * If we find one we complete the pending invoke.
	 *
	 * @param result - The result message (result part is already decoded thus we can handle the result and decode the result argument according to the function beeing called).
	 * @param invokeContext - The invoke (every result message is like an invoke) related contextual data (see IReceiveInvokeContext)
	 * @returns - Returns a reject in case of an exception or an undefined
	 */
	protected onROSEResult(result: ROSEResult, invokeContext: IReceiveInvokeContext): ROSEReject | undefined {
		try {
			const operation = this.pendingInvokes.get(result.invokeID);
			if (operation) {
				this.pendingInvokes.delete(result.invokeID);
				operation.completed_result(result);
			}
			return undefined;
		} catch (error) {
			this.log(ELogSeverity.error, "exception", "onROSEResult", this, result, error);
			return createInvokeReject(result.invokeID, InvokeProblemenum.unexpectedChildOperation, "Exception catched while handling OnROSEResult");
		}
	}

	/**
	 * The ROSE error is handled here - (we called the other side and received an error response)
	 * Based on the invoke ID we search for a pending invoke in the list of pending invokes
	 * If we find one we complete the pending invoke.
	 *
	 * @param error - The error message (error part is already decoded thus we can handle the error).
	 * @param invokeContext - The invoke (every error message is like an invoke) related contextual data (see IReceiveInvokeContext)
	 * @returns - Returns a reject in case of an exception or an undefined
	 */
	protected onROSEError(error: ROSEError, invokeContext: IReceiveInvokeContext): ROSEReject | undefined {
		try {
			const operation = this.pendingInvokes.get(error.invokedID);
			if (operation) {
				this.pendingInvokes.delete(error.invokedID);
				operation.completed_error(error);
			}
			return undefined;
		} catch (exception) {
			this.log(ELogSeverity.error, "exception", "onROSEError", this, error, exception);
			return createInvokeReject(0, InvokeProblemenum.unexpectedChildOperation, "Exception catched while handling OnROSEError");
		}
	}

	/**
	 * The ROSE reject is handled here - (we called the other side and received a reject response)
	 * Based on the invoke ID we search for a pending invoke in the list of pending invokes
	 * If we find one we complete the pending invoke.
	 *
	 * @param reject - The reject message (reject part is already decoded thus we can handle the reject).
	 * @param invokeContext - The invoke (every rehect message is like an invoke) related contextual data (see IReceiveInvokeContext)
	 * @returns - Returns a reject in case of an exception or an undefined
	 */
	protected onROSEReject(reject: ROSEReject, invokeContext: IReceiveInvokeContext): ROSEReject | undefined {
		try {
			if (reject.invokedID.invokedID !== undefined) {
				const operation = this.pendingInvokes.get(reject.invokedID.invokedID);
				if (operation) {
					this.pendingInvokes.delete(reject.invokedID.invokedID);
					operation.completed_reject(reject);
				}
			}
			return undefined;
		} catch (error) {
			this.log(ELogSeverity.error, "exception", "onROSEReject", this, reject, error);
			return createInvokeReject(0, InvokeProblemenum.unexpectedChildOperation, "Exception catched while handling OnROSEReject");
		}
	}

	/**
	 * the timeout function was called before the regular response (result, error, reject) was received
	 *
	 * @param id - id of the invoke where the timout occured
	 */
	protected onROSETimeout(id: number): void {
		const operation = this.pendingInvokes.get(id);
		if (operation) {
			this.pendingInvokes.delete(id);
			operation.complete_timedout();
		}
	}

	/**
	 * Get the textual representation of ASN1ClassInstanceType
	 *
	 * @returns the text of the enum value
	 */
	protected getASN1ClassInstanceType(): string {
		switch (this.instanceType) {
			case ASN1ClassInstanceType.TSASN1Server:
				return "TSASN1Server";
			case ASN1ClassInstanceType.TSASN1BrowserClient:
				return "TSASN1BrowserClient";
			case ASN1ClassInstanceType.TSASN1NodeClient:
				return "TSASN1NodeClient";
			default:
				debugger;
				return "";
		}
	}

	/**
	 * Retrieves the stack of the calling function
	 * We can remove some levels in case we are in a handling function and do not want to see the call to that handling function but the call before
	 *
	 * @param back - how many levels on the stack do we want to go back
	 * @returns - the stack that allows to see where a call has come from
	 */
	private static getCallStack(back = 1): IASN1CallStackEntry[] {
		const result: IASN1CallStackEntry [] = [];
		const stack = new Error().stack;
		if (stack) {
			const elements = stack.split("\n");
			for (let iCount = 2 + back; iCount++; iCount < elements.length) {
				const caller = elements[iCount];
				if (!caller)
					break;
				let bIsNode = true;
				const reg1 = / at (.*) /;
				let method = reg1.exec(caller);
				if (!method) {
					const reg2 = /(.*)@/;
					method = reg2.exec(caller);
					bIsNode = false;
				}
				if (method && method[1]) {
					let split: RegExpExecArray | null = null;
					if (bIsNode) {
						// node js root element, we stop here as the root level gives no additional insights...
						if (method[1] === "Object.<anonymous>")
							break;
						const reg3 = /(.*)\.(.*)/;
						split = reg3.exec(method[1]);
					}
					if (split)
						result.push({ class: split[1], method: split[2] });
					else
						result.push({ method: method[1] });
					if (!bIsNode) {
						// After the componentDidMount in react we need no further entires (just blurries the callstack)
						if (method[1] === "componentDidMount")
							break;
					}
				}
			}
		}
		return result;
	}
}
