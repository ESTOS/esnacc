import { TSASN1BrowserClient } from "./stub/TSASN1BrowserClient";
import { IClientConnectionCallback } from "./stub/TSASN1Client";
import {
	AsnInvokeProblem,
	CustomInvokeProblemEnum,
	ELogSeverity,
	IASN1LogCallback,
	IASN1LogData,
} from "./stub/TSROSEBase";
import { ENetUC_Common } from "./stub/types";
// Uncomment these in case you want to use the handlers as classes
// import { EventManager } from "./handlers/eventManager";
// import { SettingsManager } from "./handlers/settingsManager";
import { ENetUC_Event_ManagerROSE } from "./stub/ENetUC_Event_ManagerROSE";
import { IENetUC_Event_ManagerROSE_Handler } from "./stub/ENetUC_Event_ManagerROSE_Interface";
import { ENetUC_Settings_ManagerROSE } from "./stub/ENetUC_Settings_ManagerROSE";
import { IENetUC_Settings_ManagerROSE_Handler } from "./stub/ENetUC_Settings_ManagerROSE_Interface";
import {
	IErrorLogEntry,
	IInvokeLogEntry,
	IRejectLogEntry,
	IResultLogEntry,
	IROSELogEntryBase,
} from "./stub/TSASN1Base";
import { EASN1TransportEncoding, ISendInvokeContextParams } from "./stub/TSInvokeContext";

// These settings are provided by the .env file
const name = import.meta.env["VITE_MICROSERVICE_SERVER_LISTEN_NAME"] as string || "localhost";
const port = import.meta.env["VITE_MICROSERVICE_SERVER_LISTEN_PORT"] as string || "3020";
let wstarget = "ws";
let resttarget = "http";
if (import.meta.env["VITE_MICROSERVICE_SERVER_LISTEN_TLS"] === "1") {
	wstarget += "s";
	resttarget += "s";
}
wstarget += `://${name}:${port}/ws`;
resttarget += `://${name}:${port}/rest`;

/*
	Basically you have two different options in where to hold the different asn1 rose interfaces
	Either embedded in theClient or looosen and use theClient only as transport layer

	This basically depends on personal preference whether you want to use it class based or more function based

	Any invoke or event the server calls towards the client is either a method inside a class
	OR some method you implement where you need it
*/

/**
 * Sample client that supports a rest request mode and a websocket connection mode
 */
class Client extends TSASN1BrowserClient implements IClientConnectionCallback {
	// The different managers for the rose interfaces

	// Uncomment these in case you want to use the handlers as classes
	// public readonly eventManager: EventManager;
	// public readonly settingsManager: SettingsManager;

	// Log entries
	private results: string[] = new Array<string>(0);

	/**
	 * Constructs
	 */
	public constructor() {
		super(EASN1TransportEncoding.JSON);
		this.encodeContext.bPrettyPrint = true;

		// These are the handling modules the client is offering
		// The modules hold the rose implementation capsuled as member but use this class as transport layer
		// Uncomment these in case you want to use the handlers as classes
		// this.eventManager = new EventManager(this);
		// this.settingsManager = new SettingsManager(this);

		this.addConnectionCallback(this);
		this.setTarget(wstarget);
		this.encodeContext.bAddTypes = false;
		// Put a high timeout for debugging purposes
		this.defaultTimeout = 30000;
	}

	/**
	 * Disconnects from the server
	 */
	public override async disconnect(): Promise<void> {
		if (this.ws) {
			this.addLogEntry(`${this.getTime()} disconnecting...`, "color:darkgray");
			await super.disconnect(true);
		}
	}

	/**
	 * Sets if we connect via REST or WebSocket
	 *
	 * @param transport - sets the transport mode to either REST or WEBSOCKET (with eventing)
	 */
	public setTransport(transport: "REST" | "WEBSOCKET"): void {
		if (transport === "REST")
			this.setTarget(resttarget);
		else if (transport === "WEBSOCKET")
			this.setTarget(wstarget);
	}

	/**
	 * Clears the log entries
	 */
	public clearLog(): void {
		this.results = new Array<string>(0);
		this.addLogEntry("");
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
	public override log(
		severity: ELogSeverity,
		message: string,
		calling_method: string,
		cbOrLogData: IASN1LogCallback | IASN1LogData,
		meta?: unknown,
		exception?: unknown,
	): void {
		if (meta) {
			const logEntry = meta as IROSELogEntryBase;
			let color = logEntry.isOutbound ? "green" : "orange";
			const direction = logEntry.isOutbound ? "OUT" : "IN";
			if (logEntry._type === "IInvokeLogEntry") {
				const invoke = meta as IInvokeLogEntry;
				this.addLogEntry(
					`${this.getTime()} - ${
						invoke.isEvent ? "Event" : "Invoke"
					} - ${direction} - ${invoke.operationName} - ${invoke.argumentName}`,
					`color:${color};font-weight:bold`,
				);
				this.addLogEntry(`<pre>${JSON.stringify(invoke.argument, null, 2)}</pre>`);
			}
			else if (logEntry._type === "IResultLogEntry") {
				const result = meta as IResultLogEntry;
				this.addLogEntry(
					`${this.getTime()} - Result - ${direction} - ${result.operationName} - ${result.resultName}`,
					`color:${color}`,
				);
				this.addLogEntry(`<pre>${JSON.stringify(result.result, null, 2)}</pre>`);
			}
			else if (logEntry._type === "IRejectLogEntry") {
				const reject = meta as IRejectLogEntry;
				color = "orange";
				this.addLogEntry(`${this.getTime()} - Reject - ${direction} - ${reject.operationName}`, `color:${color}`);
				this.addLogEntry(`<pre>${JSON.stringify(reject.reject, null, 2)}</pre>`);
			}
			else if (logEntry._type === "IErrorLogEntry") {
				const error = meta as IErrorLogEntry;
				color = "red";
				this.addLogEntry(`${this.getTime()} - Error - ${direction} - ${error.operationName}`, `color:${color}`);
				this.addLogEntry(`<pre>${JSON.stringify(error.error, null, 2)}</pre>`);
			}
		}
	}

	/**
	 * TODOC
	 *
	 * @param entry - the log message to set
	 * @param css - the style to apply in case it´s needed
	 */
	public addLogEntry(entry: string, css?: string): void {
		if (css)
			entry = `<span style="${css}">${entry}</span>`;
		if (entry.length)
			this.results.push(entry);
		while (this.results.length > 50)
			this.results.shift();
		const div = document.getElementById("result");
		if (div)
			div.innerHTML = this.results.join("</br>");
		const button = document.getElementById("clear") as HTMLButtonElement;
		if (button)
			button.disabled = this.results.length ? false : true;
	}

	/**
	 * TODOC
	 *
	 * @returns - TODOC
	 */
	public getTime(): string {
		const time = new Date();
		let result = "";
		if (time.getHours() < 10)
			result += "0";
		result += time.getHours().toString();
		result += ":";
		if (time.getMinutes() < 10)
			result += "0";
		result += time.getMinutes().toString();
		result += ":";
		if (time.getSeconds() < 10)
			result += "0";
		result += time.getSeconds().toString();
		result += ":";
		if (time.getMilliseconds() < 10)
			result += "00";
		else if (time.getMilliseconds() < 100)
			result += "0";
		result += time.getMilliseconds().toString();
		return result;
	}

	/**
	 * TODOC
	 *
	 * @param bReconnected - TODOC
	 */
	public async onClientConnected(bReconnected: boolean): Promise<void> {
		this.addLogEntry(`${this.getTime()} connected to ${this.target}`, "color:darkgray");
		const button = document.getElementById("disconnect") as HTMLInputElement;
		if (button)
			button.disabled = false;
	}

	/**
	 * TODOC
	 *
	 * @param bShuttingDown - TODOC
	 */
	public async onClientDisconnected(bShuttingDown: boolean): Promise<void> {
		this.addLogEntry(`${this.getTime()} disconnected`, "color:darkgray");
		const button = document.getElementById("disconnect") as HTMLInputElement;
		if (button)
			button.disabled = true;
	}

	/**
	 * Template type guard method that ensures that we receive a result of type T
	 *
	 * @throws an ENetUC_Common.AsnRequestError object in case reject is undefined and an error occured
	 *
	 * @param response - The result as provided by the rose stub
	 * @param className - The classname that we expect as result
	 * @param reject - The reject method
	 * @returns - an object of type T
	 */
	public getResult<T>(
		response: T | ENetUC_Common.AsnRequestError | AsnInvokeProblem,
		className: { new(...args: any[]): T; },
		reject?: (reason?: ENetUC_Common.AsnRequestError) => void,
	): T {
		if (response instanceof className)
			return response;

		let error: ENetUC_Common.AsnRequestError;

		if (response instanceof ENetUC_Common.AsnRequestError)
			error = response;
		else if (response instanceof AsnInvokeProblem) {
			error = new ENetUC_Common.AsnRequestError({
				iErrorDetail: response.value || -1,
				u8sErrorString: response.details,
			});
		}
		else {
			error = new ENetUC_Common.AsnRequestError({
				iErrorDetail: CustomInvokeProblemEnum.missingResponse,
				u8sErrorString: "unhandled result",
			});
		}

		if (reject)
			reject(error);

		throw error;
	}

	/**
	 * Getter for the invoke context
	 * Allows to intercept the context that the caller has (oprovided
	 *
	 * @param context - the context the caller has provided with an invoke or event
	 * @param operationID - the operation ID that has been called
	 * @param operationName - the operation Name that has been called
	 * @param event - true, in case we are encoding an event, false for a regular invoke
	 * @returns - an updated context
	 */
	public override getInvokeContextParams(
		context: Partial<ISendInvokeContextParams> | undefined,
		operationID: number,
		operationName: string,
		event: boolean,
	): ISendInvokeContextParams {
		return { ...context, bAddOperationName: true };
	}
}

export const theClient = new Client();

// We hold the settingsManager outside of the client object, you could also make it a member of the client object
export const settingsManager: ENetUC_Settings_ManagerROSE & Partial<IENetUC_Settings_ManagerROSE_Handler> =
	new ENetUC_Settings_ManagerROSE(theClient, true);
// The settingsManager itself also implements the receiver interface (events, server to client invokes)
settingsManager.setHandler(settingsManager);

// We hold the settingsManager outside of the client object, you could also make it a member of the client object
export const eventManager: ENetUC_Event_ManagerROSE & Partial<IENetUC_Event_ManagerROSE_Handler> =
	new ENetUC_Event_ManagerROSE(theClient, true);
// The settingsManager itself also implements the receiver interface (events, server to client invokes)
eventManager.setHandler(eventManager);
