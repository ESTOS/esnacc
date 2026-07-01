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
 * A structured log entry that can optionally carry metadata.
 */
interface ILogEntry {
	id: number;
	html: string; // the main line (may contain styled <span>)
	meta?: string; // pretty-printed JSON string, present only when metadata exists
}

/**
 * Sample client that supports a rest request mode and a websocket connection mode
 */
class Client extends TSASN1BrowserClient implements IClientConnectionCallback {
	// The different managers for the rose interfaces

	// Uncomment these in case you want to use the handlers as classes
	// public readonly eventManager: EventManager;
	// public readonly settingsManager: SettingsManager;

	// Structured log entries
	private results: ILogEntry[] = [];
	private nextId = 0;
	public loglevel: ELogSeverity | number = 0;

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
		this.addLogEntry(`${this.getTime()} disconnecting...`, "color:darkgray");
		return super.disconnect(true);
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
		this.results = [];
		this.renderLog();
	}

	/**
	 * Change how we handle auto reconnect
	 *
	 * @param autoReconnect - true to enable automatic reconnect (timer based) or false to disable it
	 */
	public setAutoReconnect(autoReconnect: boolean): void {
		if (!autoReconnect)
			this.setReconnectTimeout(0);
		this.autoReconnect = autoReconnect;
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
			let color = logEntry.isOutbound ? "green" : "blue";
			const direction = logEntry.isOutbound ? "OUT" : "IN";
			if (logEntry._type === "IInvokeLogEntry") {
				const invoke = meta as IInvokeLogEntry;
				this.addLogEntry(
					`${this.getTime()} - ${
						invoke.isEvent ? "EVENT" : "INVOKE"
					} - ${direction} - ${invoke.operationName} - ${invoke.argumentName}`,
					`color:${color};font-weight:bold`,
					JSON.stringify(invoke.argument, null, 2),
				);
				return;
			} else if (logEntry._type === "IResultLogEntry") {
				const result = meta as IResultLogEntry;
				this.addLogEntry(
					`${this.getTime()} - RESULT - ${direction} - ${result.operationName} - ${result.resultName}`,
					`color:${color}`,
					JSON.stringify(result.result, null, 2),
				);
				return;
			} else if (logEntry._type === "IRejectLogEntry") {
				const reject = meta as IRejectLogEntry;
				color = "orange";
				this.addLogEntry(
					`${this.getTime()} - REJECT - ${direction} - ${reject.operationName}`,
					`color:${color}`,
					JSON.stringify(reject.reject, null, 2),
				);
				return;
			} else if (logEntry._type === "IErrorLogEntry") {
				const error = meta as IErrorLogEntry;
				color = "red";
				this.addLogEntry(
					`${this.getTime()} - ERROR - ${direction} - ${error.operationName}`,
					`color:${color}`,
					JSON.stringify(error.error, null, 2),
				);
				return;
			}
		}
		if (severity <= this.loglevel) {
			let color = "";
			let type = "";
			switch (severity) {
				case ELogSeverity.debug:
					color = "darkgrey";
					type = "debug";
					break;
				case ELogSeverity.error:
					color = "darkred";
					type = "error";
					break;
				case ELogSeverity.warn:
					color = "darkorange";
					type = "warn";
					break;
				case ELogSeverity.info:
					color = "black";
					type = "info";
					break;
				default:
					color = "purple";
					type = "unknown";
					break;
			}
			const logMessage = `${this.getTime()} - ${type} - ${calling_method} - ${message}`;
			this.addLogEntry(logMessage, `color:${color}`, meta ? JSON.stringify(meta, null, 2) : undefined);
			return;
		}
	}

	/**
	 * Adds a structured log entry to the UI by appending a new DOM node.
	 * Existing nodes are never touched, so expanded metadata panels stay open
	 * when new entries arrive.
	 *
	 * When metaJson is provided the entry renders an expand/collapse toggle
	 * button *before* the timestamp; entries without metadata show an
	 * invisible placeholder so all timestamps stay aligned.
	 *
	 * @param html     - the main log line content (plain text or trusted HTML)
	 * @param css      - optional inline style applied to the main line
	 * @param metaJson - optional pretty-printed JSON shown in an expandable block
	 */
	public addLogEntry(html: string, css?: string, metaJson?: string): void {
		if (!html.length)
			return;

		const div = document.getElementById("result");
		if (!div)
			return;

		const id = this.nextId++;
		this.results.push({ id, html, meta: metaJson });

		// Enforce the 50-entry cap by removing the oldest DOM node first.
		while (this.results.length > 50) {
			this.results.shift();
			const oldest = div.firstElementChild;
			if (oldest)
				div.removeChild(oldest);
		}

		// Build the new entry node entirely in the DOM - no innerHTML on the container.
		const entryEl = document.createElement("div");
		entryEl.className = "log-entry";

		const lineEl = document.createElement("div");
		lineEl.className = "log-entry__line";

		if (metaJson) {
			// Active toggle button
			const btn = document.createElement("button");
			btn.type = "button"; // prevent form submission - default is "submit" inside <form>
			btn.id = `log-btn-${id}`;
			btn.className = "log-toggle";
			btn.title = "Expand metadata";
			btn.textContent = "▶";
			btn.addEventListener("click", () => {
				toggleLogMeta(`log-meta-${id}`, btn);
			});
			lineEl.appendChild(btn);

			// Metadata panel (hidden by default)
			const metaEl = document.createElement("div");
			metaEl.id = `log-meta-${id}`;
			metaEl.className = "log-meta";
			metaEl.hidden = true;
			const pre = document.createElement("pre");
			pre.textContent = metaJson; // textContent handles escaping automatically
			metaEl.appendChild(pre);
			entryEl.appendChild(metaEl); // appended to entry, revealed below the line
		} else {
			// Invisible spacer - keeps timestamps aligned with entries that have a button
			const spacer = document.createElement("button");
			spacer.className = "log-toggle log-toggle--empty";
			spacer.setAttribute("aria-hidden", "true");
			lineEl.appendChild(spacer);
		}

		// Main text - set via innerHTML so callers can pass styled <span> content
		const textEl = document.createElement("span");
		textEl.innerHTML = css ? `<span style="${css}">${html}</span>` : html;
		lineEl.appendChild(textEl);

		// Line comes first, meta panel (if any) is already appended; reorder so
		// the line is the first child and meta is below it.
		entryEl.insertBefore(lineEl, entryEl.firstChild);

		div.appendChild(entryEl);

		const clearBtn = document.getElementById("clear") as HTMLButtonElement;
		if (clearBtn)
			clearBtn.disabled = false;
	}

	/**
	 * Clears all DOM nodes from the log container and resets internal state.
	 */
	private renderLog(): void {
		const div = document.getElementById("result");
		if (!div)
			return;
		div.innerHTML = "";
		const clearBtn = document.getElementById("clear") as HTMLButtonElement;
		if (clearBtn)
			clearBtn.disabled = true;
	}

	/**
	 * Retrieve a predefined time for the log messages
	 *
	 * @returns - the time as string
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
	 * Is called in case the client is not connected and about to establish a statefull connection
	 * Within that callback the target may get changed (e.g. if you need auth data for the websocket connection)
	 * Returning false will terminate the connect request (and all pending connect request)
	 *
	 * @param bReconnecting - true in case we are reconnecting
	 */
	public async onBeforeConnect(bReconnecting: boolean): Promise<boolean> {
		return true;
	}

	/**
	 * The client is now connected
	 *
	 * @param bReconnected - true in case it was a reconnect
	 */
	public async onClientConnected(bReconnected: boolean): Promise<void> {
		this.addLogEntry(`${this.getTime()} connected to ${this.target}`, "color:darkgray");
		this.adoptUIElements(true);
	}

	/**
	 * The client is now disconnected
	 *
	 * @param bShuttingDown - true in case it was intentional and not by a connection loss
	 */
	public async onClientDisconnected(bShuttingDown: boolean): Promise<void> {
		this.addLogEntry(`${this.getTime()} disconnected`, "color:darkgray");
		this.adoptUIElements(false);
	}

	/**
	 * Enable or disables the UI elements based on the connection state
	 *
	 * @param bConnected - true if we are connected, or false if we are disconnected
	 */
	private adoptUIElements(bConnected: boolean): void {
		const connectionState = document.getElementById("state");
		if (connectionState)
			connectionState.textContent = bConnected ? "Connected" : "Connecting";
		const elements = ["disconnect", "encoding", "transport"];
		for (const element of elements) {
			const button = document.getElementById(element) as HTMLInputElement;
			if (button)
				button.disabled = element === "disconnect" ? !bConnected : bConnected;
		}
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
	): T | undefined {
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
		} else {
			error = new ENetUC_Common.AsnRequestError({
				iErrorDetail: CustomInvokeProblemEnum.missingResponse,
				u8sErrorString: "unhandled result",
			});
		}

		return undefined;
	}

	/**
	 * Getter for the invoke context
	 * Allows to intercept the context that the caller has (provided)
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

// -- Helpers -----------------------------------------------------------------

/**
 * Toggles a metadata panel open/closed and updates the arrow on the button.
 * Called directly via closure from the button's addEventListener - no global
 * window attachment needed.
 *
 * @param metaId - the id for the element we want to show or hide
 * @param btn - the corresponding button element
 */
function toggleLogMeta(metaId: string, btn: HTMLButtonElement): void {
	const meta = document.getElementById(metaId);
	if (!meta)
		return;
	if (meta.hidden) {
		meta.hidden = false;
		btn.textContent = "▼";
		btn.title = "Collapse metadata";
	} else {
		meta.hidden = true;
		btn.textContent = "▶";
		btn.title = "Expand metadata";
	}
}

// -- Exports ------------------------------------------------------------------

export const theClient = new Client();

// We hold the settingsManager outside of the client object, you could also make it a member of the client object
export const settingsManager: ENetUC_Settings_ManagerROSE & Partial<IENetUC_Settings_ManagerROSE_Handler> =
	new ENetUC_Settings_ManagerROSE(theClient, true);
// The settingsManager itself also implements the receiver interface (events, server to client invokes)
settingsManager.setHandler(settingsManager);

// We hold the eventManager outside of the client object, you could also make it a member of the client object
export const eventManager: ENetUC_Event_ManagerROSE & Partial<IENetUC_Event_ManagerROSE_Handler> =
	new ENetUC_Event_ManagerROSE(theClient, true);
// The eventManager itself also implements the receiver interface (events, server to client invokes)
eventManager.setHandler(eventManager);
