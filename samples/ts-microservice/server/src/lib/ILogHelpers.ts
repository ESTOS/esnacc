import { ILogCallback, ILogData } from "uclogger";
import WebSocket from "ws";

import { EOwnInterval, EOwnTimeout } from "./common_timers.js";

// Loglevels we support
export type LogLevels = "error" | "warn" | "info" | "debug";

/**
 * Interface to access theLogger
 */
export interface ILogger {
	error(
		msg: string,
		calling_method: string,
		logdata_or_callback?: ILogData | ILogCallback,
		meta?: unknown,
		exception?: unknown,
	): void;
	warn(
		msg: string,
		calling_method: string,
		logdata_or_callback?: ILogData | ILogCallback,
		meta?: unknown,
		exception?: unknown,
	): void;
	info(
		msg: string,
		calling_method: string,
		logdata_or_callback?: ILogData | ILogCallback,
		meta?: unknown,
		exception?: unknown,
	): void;
	debug(
		msg: string,
		calling_method: string,
		logdata_or_callback?: ILogData | ILogCallback,
		meta?: unknown,
		exception?: unknown,
	): void;
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
type logany = any;

/* eslint-disable @typescript-eslint/no-unsafe-assignment */
/* eslint-disable @typescript-eslint/no-unsafe-member-access */
/**
 * This function removes rather large objects from a log object which bring no benefit to the log output
 * e.g. a websocket requires 100k json log, but has no sense to be logged, same for the timer objects
 * @param obj - the object to parse
 * @param levelstoprocess - a counter that is used to only process a certain level (deepness in the object)
 * @param id - in recursion the id of the parent element we are currently handling
 * @param cache - an internal cache object for the recursive calls
 * @returns - the cleaned object
 */
function omitForLoggingInternal(obj: logany, levelstoprocess?: number, id?: string, cache: logany[] = []): logany {
	try {
		let result: logany | undefined;

		const type = typeof obj;
		switch (type) {
			case "boolean":
			case "number":
			case "string":
			case "bigint":
			case "undefined":
				result = obj;
				break;
			case "function":
			case "symbol":
				// we do not take over symbols or functions into the Logger
				break;
			case "object":
				if (obj instanceof Date)
					result = obj;
				else if (obj instanceof WebSocket || obj instanceof EOwnTimeout || obj instanceof EOwnInterval)
					result = omitForLoggingInternal(obj, 0, id, cache);
				else if (obj === null)
					result = obj;
				else {
					if (obj.toJSON)
						obj = obj.toJSON();

					if (cache.indexOf(obj) !== -1)
						result = "circular_reference";
					else {
						cache.push(obj);

						let counter = 0;
						result = {};
						const keys = Object.keys(obj);
						for (const key of keys) {
							if (counter < 20 || levelstoprocess === undefined) {
								const element = obj[key];
								if (levelstoprocess === undefined)
									result[key] = omitForLoggingInternal(element, levelstoprocess, key, cache);
								else if (levelstoprocess > 0)
									result[key] = omitForLoggingInternal(element, levelstoprocess - 1, key, cache);
								else {
									const type = typeof element;
									if (type === "object")
										result[key] = "obj truncated";
									else if (type === "function")
										result[key] = "func truncated";
									else if (type === "symbol")
										result[key] = "sym truncated";
									else
										result[key] = element;
								}
								counter++;
							}
							else if (counter === 20) {
								result.truncated = true;
								counter++;
							}
						}
					}
				}
				break;
			default:
				// die we miss something?

				debugger;
				break;
		}

		return result;
	}
	catch (error) {
		console.error(error);
		debugger;
		return obj;
	}
}

/**
 * This function removes rather large objects from a log object which bring no benefit to the log output
 * e.g. a websocket requires 100k json log, but has no sense to be logged, same for the timer objects
 * @param obj - the object to parse
 * @param levelstoprocess - a counter that is used to only process a certain level (deepness in the object)
 * @returns - the cleaned object
 */
export function omitForLogging(obj: logany, levelstoprocess?: number): logany {
	return omitForLoggingInternal(obj, levelstoprocess);
}
