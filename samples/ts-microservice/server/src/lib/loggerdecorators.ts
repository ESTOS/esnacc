import { ILogCallback, ILogData, LogLevels } from "uclogger";

import { ILogger, omitForLogging } from "./ILogHelpers";

// By default do not log arguments and results, just the function call and the result
const defaultNoArgumentsAndResult = true;

// Global Invoke Result counter for the decorator based logging
let gCallCounter = 0;

// This is the interface to a logger call (debug, error etc.)
type loggerinterface = (msg: string, calling_method: string, logData_or_Callback?: ILogData | ILogCallback, meta?: unknown, exception?: never) => void;
let bSetDecoratorLoggerCalled = false;
let gLogError: loggerinterface | undefined;
let gLogWarn: loggerinterface | undefined;
let gLogInfo: loggerinterface | undefined;
let gLogDebug: loggerinterface | undefined;

/**
 * Allows to set the logger for the log decorators
 *
 * @param logger - an ILogger interface that is used within the logger decorators
 */
export function setDecoratorLogger(logger: ILogger): void {
	bSetDecoratorLoggerCalled = true;
	gLogError = logger.error.bind(logger);
	gLogWarn = logger.warn.bind(logger);
	gLogInfo = logger.info.bind(logger);
	gLogDebug = logger.debug.bind(logger);
}

// Internal CallData structure
interface ICallData {
	callCounter: number;
	data?: unknown;
}

// Internal ResultData structure
interface IResultData {
	callCounter: number;
	promise?: true;
	result?: unknown;
	reject?: unknown;
	duration?: number;
}

// Internal strucutre that is handed back from the logCall
interface ILogInternal {
	logger: loggerinterface;
	callCounter: number;
	methodName: string;
	logCallback: ILogCallback;
	now: number;
}

/**
 * To ease debugging through the decorators we try to move all code into two dedicated methods that allow to step over the code easily
 * This method logs the call and returns a structure that is reused in the logResult
 *
 * @param level - the logger type that has been called (error, debug etc)
 * @param methodName - the method name calling the logger
 * @param bNoArgumentValues - true if the arguments shall not be logged
 * @param logCallback - A callback (normally the class calling the method) to retrieve additional data for the logging
 * @param args - the variadic argument list the original call was handed over
 * @returns the interface for the requested logger method (error, debug etc)
 */
function logCall(level: LogLevels | "special", methodName: string, bNoArgumentValues: boolean, logCallback: unknown, args: unknown[]): ILogInternal {
	if (!bSetDecoratorLoggerCalled) {
		// If you reach this you did call theLogger before it has been initialized.
		// Check the call stack
		// Do not use theLogger inside singleton constructors
		// Do not use theLogger in functions that are called from the constructors (also check decorators!)

		debugger;
		throw new Error("Never use theLogger before it has been initialized (e.g. singleton constructors)");
	}
	if (!logCallback) {
		// If this is not callable, the decorator was called out of the classes context.
		// You must ensure that the function you call is bound to the class .bind(this);

		debugger;
		throw new Error("Decorator functions must be bound to the class. Otherwise the decorator cannot call the member function!");
	}

	let logger: loggerinterface | undefined;
	switch (level) {
		case "error":
			logger = gLogError;
			break;
		case "warn":
			logger = gLogWarn;
			break;
		case "info":
			logger = gLogInfo;
			break;
		case "debug":
			logger = gLogDebug;
			break;
		default:
			debugger;
			logger = gLogError;
			break;
	}
	if (!logger) {
		debugger;
		throw new Error(`unhandled loglevel ${level} was specific`);
	}

	const calldata: ICallData = {
		callCounter: ++gCallCounter
	};

	if (!bNoArgumentValues && args.length) {
		const arg = args.length === 1 ? args[0] : args;
		if (arg)
			calldata.data = omitForLogging(arg, 3);
	}

	logger("call", methodName, (logCallback as ILogCallback), calldata);

	return {
		logger,
		callCounter: calldata.callCounter,
		methodName,
		logCallback: (logCallback as ILogCallback),
		now: Date.now()
	};
}

/**
 * To ease debugging through the decorators we try to move all code into two dedicated methods that allow to step over the code easily
 * This method logs the result of the function call
 *
 * @param logInternal - Internal data that is provided by the logCall
 * @param bNoResultValues - true if the result shall not be logged
 * @param result - the original result of the method beeing logged by the decorator
 * @returns the result or the intercepted promise
 */
function logResult(logInternal: ILogInternal, bNoResultValues: boolean, result: unknown): unknown {
	if (result instanceof Promise) {
		const originalResult = result;
		result = new Promise((resolve, reject): void => {
			originalResult.then((res: unknown) => {
				const duration = Date.now() - logInternal.now;
				if (duration > 100) {
					const resultdata: IResultData = {
						callCounter: logInternal.callCounter,
						promise: true,
						duration
					};
					if (!bNoResultValues && res)
						resultdata.result = omitForLogging(res, 2);
					logInternal.logger("return", logInternal.methodName, logInternal.logCallback, resultdata);
				}
				resolve(res);
			}).catch((rej: unknown) => {
				const resultdata = {
					callCounter: logInternal.callCounter,
					promise: true,
					reject: rej,
					duration: Date.now() - logInternal.now
				};
				logInternal.logger("return", logInternal.methodName, logInternal.logCallback, resultdata);
				reject(rej);
			});
		});
	} else {
		const duration = Date.now() - logInternal.now;
		if (duration > 100) {
			const resultdata: IResultData = {
				callCounter: logInternal.callCounter,
				duration
			};
			if (!bNoResultValues && result)
				resultdata.result = omitForLogging(result, 2);

			logInternal.logger("return", logInternal.methodName, logInternal.logCallback, resultdata);
		}
	}
	return result;
}

/**
 * Method that actually logs one of the log decorators
 *
 * @param target - Either the constructor function of the class for a static method, or the prototype of the class for an instance method.
 * @param methodName - The methods name
 * @param propertyDesciptor - The Property Descriptor for the method
 * @param type - the logger type that has been called (error, debug etc)
 * @param bNoArgumentValues - true if the arguments shall not be logged
 * @param bNoResultValues - true if the result shall not be logged
 * @returns - The Property Descriptor for the method
 */
function logMethod(
	target: Record<string, unknown>,
	methodName: string,
	propertyDesciptor: PropertyDescriptor,
	type: LogLevels | "special",
	bNoArgumentValues = defaultNoArgumentsAndResult,
	bNoResultValues = defaultNoArgumentsAndResult): PropertyDescriptor {
	/* eslint-disable @typescript-eslint/no-unsafe-assignment */
	const method = propertyDesciptor.value;

	/**
	 * The internal decorator function that is called when the function itself is called
	 *
	 * @param args - Arguments of the decorator function
	 * @returns - the methods result property descriptor
	 */
	propertyDesciptor.value = function(...args: unknown[]): unknown {
		const logInternal = logCall(type, methodName, bNoArgumentValues, (this as ILogCallback), args);

		// Here the original function is beeing called...
		/* eslint-disable-next-line @typescript-eslint/no-unsafe-member-access */
		const result = method.apply(this, args);

		return logResult(logInternal, bNoResultValues, result);
	};
	/* eslint-enable @typescript-eslint/no-unsafe-assignment */
	return propertyDesciptor;
}

/**
 * Empty decorator if we do not want to log the function call but want to have a decorator for linting
 *
 * @param target - Either the constructor function of the class for a static method, or the prototype of the class for an instance method.
 * @param methodName - The methods name
 * @param propertyDesciptor - The Property Descriptor for the method
 * @returns - The Property Descriptor for the method
 */
export function logNothing(
	target: Record<string, unknown>,
	methodName: string,
	propertyDesciptor: PropertyDescriptor): PropertyDescriptor {
	return propertyDesciptor;
}

/**
 * Decorator that logs a function call/result and takes care of the embedding class might be a ILogBase derived class (Adds identifiers from these classes)
 * Logs as debug message
 * Log with this level if you need debug information
 *
 * @param target - Either the constructor function of the class for a static method, or the prototype of the class for an instance method.
 * @param methodName - The methods name
 * @param propertyDesciptor - The Property Descriptor for the method
 * @returns - The Property Descriptor for the method
 */
export function logDebug(
	target: Record<string, unknown>,
	methodName: string,
	propertyDesciptor: PropertyDescriptor): PropertyDescriptor {
	return logMethod(target, methodName, propertyDesciptor, "debug");
}

/**
 * Same as above but optionally no Argument and or no Result in the Log
 *
 * @param noArgument - do not log the argument of the method
 * @param noResult - do not log the result of the method
 * @returns - The Property Descriptor for the method
 */
export function logDebugEx(noArgument: boolean, noResult: boolean) {
	return function(target: Record<string, unknown>, methodName: string, propertyDesciptor: PropertyDescriptor): PropertyDescriptor {
		return logMethod(target, methodName, propertyDesciptor, "debug", noArgument, noResult);
	};
}

/**
 * Decorator that logs a function call/result and takes care of the embedding class might be a ILogBase derived class (Adds identifiers from these classes)
 * Logs as error message
 *
 * @param target - Either the constructor function of the class for a static method, or the prototype of the class for an instance method.
 * @param methodName - The methods name
 * @param propertyDesciptor - The Property Descriptor for the method
 * @returns - The Property Descriptor for the method
 */
export function logError(
	target: Record<string, unknown>,
	methodName: string,
	propertyDesciptor: PropertyDescriptor): PropertyDescriptor {
	return logMethod(target, methodName, propertyDesciptor, "error");
}

/**
 * Same as above but optionally no Argument and or no Result in the Log
 *
 * @param noArgument - do not log the argument of the method
 * @param noResult - do not log the result of the method
 * @returns - The Property Descriptor for the method
 */
export function logErrorEx(noArgument: boolean, noResult: boolean) {
	return function(target: Record<string, unknown>, methodName: string, propertyDesciptor: PropertyDescriptor): PropertyDescriptor {
		return logMethod(target, methodName, propertyDesciptor, "error", noArgument, noResult);
	};
}

/**
 * Decorator that logs a function call/result and takes care of the embedding class might be a ILogBase derived class (Adds identifiers from these classes)
 * Logs as info message
 * With info you log things which you want to see in a regular logfile not in debugging mode
 *
 * @param target - Either the constructor function of the class for a static method, or the prototype of the class for an instance method.
 * @param methodName - The methods name
 * @param propertyDesciptor - The Property Descriptor for the method
 * @returns - The Property Descriptor for the method
 */
export function logInfo(
	target: Record<string, unknown>,
	methodName: string,
	propertyDesciptor: PropertyDescriptor): PropertyDescriptor {
	return logMethod(target, methodName, propertyDesciptor, "info");
}

/**
 * Same as above but optionally no Argument and or no Result in the Log
 *
 * @param noArgument - do not log the argument of the method
 * @param noResult - do not log the result of the method
 * @returns - The Property Descriptor for the method
 */
export function logInfoEx(noArgument: boolean, noResult: boolean) {
	return function(target: Record<string, unknown>, methodName: string, propertyDesciptor: PropertyDescriptor): PropertyDescriptor {
		return logMethod(target, methodName, propertyDesciptor, "info", noArgument, noResult);
	};
}

/**
 * Decorator that logs a function call/result and takes care of the embedding class might be a ILogBase derived class (Adds identifiers from these classes)
 * Logs as warn message
 *
 * @param target - Either the constructor function of the class for a static method, or the prototype of the class for an instance method.
 * @param methodName - The methods name
 * @param propertyDesciptor - The Property Descriptor for the method
 * @returns - The Property Descriptor for the method
 */
export function logWarn(
	target: Record<string, unknown>,
	methodName: string,
	propertyDesciptor: PropertyDescriptor): PropertyDescriptor {
	return logMethod(target, methodName, propertyDesciptor, "warn");
}

/**
 * Same as above but optionally no Argument and or no Result in the Log
 *
 * @param noArgument - do not log the argument of the method
 * @param noResult - do not log the result of the method
 * @returns - The Property Descriptor for the method
 */
export function logWarnEx(noArgument: boolean, noResult: boolean) {
	return function(target: Record<string, unknown>, methodName: string, propertyDesciptor: PropertyDescriptor): PropertyDescriptor {
		return logMethod(target, methodName, propertyDesciptor, "warn", noArgument, noResult);
	};
}

/**
 * Decorator that logs a function call/result and takes care of the embedding class might be a ILogBase derived class (Adds identifiers from these classes)
 * Logs as debug message
 * DO NOT use this logger for production, only use it if you want to debug the logging decorator of a method and need a breakpoint for this special method
 *
 * @param target - Either the constructor function of the class for a static method, or the prototype of the class for an instance method.
 * @param methodName - The methods name
 * @param propertyDesciptor - The Property Descriptor for the method
 * @returns - The Property Descriptor for the method
 */
export function logSpecial(
	target: Record<string, unknown>,
	methodName: string,
	propertyDesciptor: PropertyDescriptor): PropertyDescriptor {
	return logMethod(target, methodName, propertyDesciptor, "special");
}
