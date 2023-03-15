import { AsyncLocalStorage } from "async_hooks";
import { IFinalLogData, ILogData } from "uclogger";
import { theConfig, theLogger } from "../globals";
import { Common } from "../lib/common";

/**
 * These are static properties we store in the client connection as they are static in association to the client connection
 */
export interface ILogContextStaticData {
	// ID the client is using within one session stays the same as long as the browser does not close the tab
	clientID?: string;
	// the client connection id on the server side (websocket connection id)
	clientConnectionID?: string;
}

/**
 * These are dynamic properties we enrich the log context with on a per invoke basis
 */
export interface ILogContextDynamicData {
	// The operation the client did call (to follow the invoke through the server based on the called method)
	operationName: string;
	// The invokeID the client used to call the server (in association with the clientID not 100% exact but a good match, in a reconnect the same combination may occur)
	invokeID: number;
}

/**
 * The class that holds the properties of the ILogContextData
 */
export class LogContextStaticData implements ILogContextStaticData {
	public clientID?: string;
	public clientConnectionID?: string;
	/**
	 * Constructs the LogContextData object
	 *
	 * @param args - Arguments the object will be initialized with, if a mandatory one is missing we prefill with default values
	 */
	public constructor(args?: Partial<ILogContextStaticData>) {
		this.clientID = args?.clientID;
		this.clientConnectionID = args?.clientConnectionID;
	}
}

export interface ILogContextData extends ILogContextStaticData, Partial<ILogContextDynamicData> {
}

interface ILogCall {
	count: number;
	totalSize: number;
}

/**
 * The async local storage containing properties of the type ILogContextData
 * While processing a client invoke we add data to the localStorage to have them available
 * wherever the sever handles the request. (Especially for logging)
 */
export class LogLocalStorage {
	// The singleton instance of this class
	private static instance: LogLocalStorage;
	// Diagnostic for the logging
	private callers = new Map<string, ILogCall>();
	// Counter that drops callers to the console
	private iLogCounter = 0;
	// The storage we add our data to
	private storage: AsyncLocalStorage<ILogContextData>;

	/**
	 * Constructs the LogLocalStorage object
	 */
	private constructor() {
		this.storage = new AsyncLocalStorage();
	}

	/**
	 * Gets instance of LogLocalStorage to use as singleton.
	 *
	 * @returns - an instance of this class.
	 */
	public static getInstance(): LogLocalStorage {
		if (!LogLocalStorage.instance)
			LogLocalStorage.instance = new LogLocalStorage();
		return LogLocalStorage.instance;
	}

	/**
	 * Initializes the LogLocalStorage object
	 */
	public init(): void {
		theLogger.setCallback(this.callback.bind(this));
	}

	/**
	 * Initializes the LogLocalStorage object
	 */
	public exit(): void {
	}

	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 *
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public getLogData(): ILogData {
		return {
			className: this.constructor.name
		};
	}

	/**
	 * Add data to the storage
	 *
	 * @param store - the data to add to the store
	 */
	public enterWith(store: ILogContextData): void {
		this.storage.enterWith(store);
	}

	/**
	 * A callback from theLogger that adds data from the LogLocalStorage to the log request
	 *
	 * @param logData - the logData as provided by theLogger
	 * @returns - enriched logData
	 */
	public callback(logData: IFinalLogData): IFinalLogData {
		const store = this.storage.getStore();
		if (store) {
			if (!logData.classProps)
				logData.classProps = { store };
			else {
				logData.classProps = {
					...store,
					...logData.classProps
				};
			}
		}

		if (theConfig.environment === "development") {
			// ONLY in development mode we do some further analysis on the stuff we drop into the log
			// - Check if required properties are there
			// - Check if a classname is known
			// - some analytics on the callers (call counter, logamount, etc)

			const con = console;

			// Check if everything is there....
			/*
				if (logData.className === undefined) {
					// check where this call comes from and why the className Property is not filled
					con.error("LogLocalStorage.callback - undefined className");
					debugger;
				}
			*/

			const caller = `${logData.className}.${logData.method}`;

			/*
				if (!store) {
					// check where this call comes from and why the className Property is not filled
					con.warn(`Found no store for a call from ${caller}`);
				}
			*/

			/*
				const test = (logData.classProps as ILogContextData);
				if (test && !test.clientID && !test.partyID && !test.roomID && !test.roomRuntimeID && logData.classProps?.parentClass !== "ConferenceController") {
					// To map log entries we need to ensure that we have properties to map them to an existing conference or client
					con.warn(`${caller} missing props clientID, partyID, roomID, roomRuntimeID`);
				}
			*/

			let datalen = 0;
			try {
				datalen = Common.stringify(logData).length;
			} catch (error) {
				debugger;
				// If we hit this line we likely have a circular dependency in the logData that needs to be resolved/removed
				// Check especially the meta data for timer objects
				con.error(error);
			}

			// We log
			const callCount: ILogCall = this.callers.get(caller) || { count: 0, totalSize: 0 };
			callCount.count++;
			callCount.totalSize += datalen;
			this.callers.set(caller, callCount);

			if (theConfig.logStatisticsOnConsole) {
				this.iLogCounter++;
				if (this.iLogCounter % 500 === 0) {
					interface ISortElement {
						method: string;
						count: number;
						avg_data: number;
						total_data: number;
					}
					const elements: ISortElement[] = [];
					for (const [key, caller] of this.callers)
						elements.push({ method: key, count: caller.count, avg_data: Math.round(caller.totalSize / caller.count), total_data: caller.totalSize });

					con.log("------------------------------");
					con.log(`Status after ${this.iLogCounter} log calls:`);
					con.log("---\nmost called:");
					elements.sort((a: ISortElement, b: ISortElement): number => { return b.count - a.count; });
					let iCount = 0;
					for (const element of elements) {
						con.log(`${element.count}x -> ${element.method}`);
						iCount++;
						if (iCount === 10)
							break;
					}

					con.log("---\nhighest average payload:");
					elements.sort((a: ISortElement, b: ISortElement): number => { return b.avg_data - a.avg_data; });
					iCount = 0;
					for (const element of elements) {
						con.log(`${element.avg_data} bytes -> ${element.method}`);
						iCount++;
						if (iCount === 10)
							break;
					}

					con.log("---\nhighest total payload:");
					elements.sort((a: ISortElement, b: ISortElement): number => { return b.total_data - a.total_data; });
					iCount = 0;
					for (const element of elements) {
						con.log(`${element.total_data} bytes -> ${element.method}`);
						iCount++;
						if (iCount === 10)
							break;
					}

					con.log("------------------------------\n");
				}
			}
		}

		return logData;
	}
}
