import { format } from "date-fns";
import fs from "fs";
import path from "path";
import { ILogData } from "uclogger";
import { getHeapSnapshot } from "v8";

import { Common } from "./common";
import { theConfig, theClientConnectionManager, theServer, theLogger, theLogStorage } from "../globals";

enum CONF_EXIT_CODES {
	EXIT_CODE_UNHANDLED_PROMISE = 64,
	EXIT_CODE_UNHANDLED_EXCEPTION = 65,
	EXIT_CODE_INIT = 66,
	EXIT_CODE_MISSING_ENV_PROPERTY = 67,
	EXIT_CODE_REDIS_END_NOTIFIED = 68,
	EXIT_CODE_REDIS_INIT_FAILED = 69,
	EXIT_CODE_DETECT_MAIN_NETWORKINTERFACE_FAILED = 70,
	EXIT_CODE_CONFIGURATION_ERROR = 71,
	EXIT_CODE_NOT_LISTENING = 72
}

const exitHandler: ILogData = { className: "exitHandler" };

/**
 * Init exit listeners
 */
const initExitHandler = function(): void {
	// Is set to true when all logs are flushed and services are shutdown
	let cleaned = false;

	// In case an error happens during shutdown ignore
	let shuttingDown = false;

	const performExit = async (dump = false, exitCauseError: Error | undefined = undefined, exitCauseReason: unknown | null | undefined = undefined): Promise<void> => {
		if (shuttingDown)
			return;
		shuttingDown = true;

		if (cleaned)
			return;

		if (dump) {
			const timeStampString = `${format(new Date(), "yyyy.MM.dd-HHmmss")}`;
			const stackFile = path.join(theConfig.logDirectory, `dump_${timeStampString}_stacktrace.txt`);
			const heapFile = path.join(theConfig.logDirectory, `dump_${timeStampString}_heapsnapshot.dmp`);

			if (exitCauseError != null)
				fs.writeFileSync(stackFile, exitCauseError.stack || exitCauseError.message);
			else if (exitCauseReason && exitCauseReason instanceof Error)
				fs.writeFileSync(stackFile, exitCauseReason.stack || exitCauseReason.message);
			else if (exitCauseReason && typeof exitCauseReason === "object")
				fs.writeFileSync(stackFile, Common.stringify(exitCauseReason));

			try {
				const snapShot = getHeapSnapshot();
				const fileStream = fs.createWriteStream(heapFile);
				snapShot.pipe(fileStream);
				fileStream.close();
			} catch (error) {
				theLogger.error("Failed to create dump", "perfomExit", exitHandler, undefined, error);
			}
		}

		theServer.exit();
		theClientConnectionManager.exit();
		theLogStorage.exit();

		cleaned = true;
		try {
			await theLogger.exit();
		} catch (error) {
			(console as Console).error(`Failed to flush logger ${error}`);
		}
	};

	/**
	 * The 'unhandledRejection' event is emitted whenever a Promise is rejected and no error handler
	 * is attached to the promise within a turn of the event loop. When programming with Promises,
	 * exceptionsare encapsulated as "rejected promises". Rejections can be caught and handled using
	 * promise.catch() and are propagated through a Promise chain. The 'unhandledRejection' event is
	 * useful for detecting and keeping track of promises that were rejected whose rejections
	 * have not yet been handled.
	 *
	 * once to prefent error loops
	 *
	 * Details: https://nodejs.org/docs/latest-v12.x/api/process.html#process_event_unhandledrejection
	 */
	process.once("unhandledRejection", async (reason: {} | null | undefined, promise: Promise<unknown>) => {
		debugger;
		if (shuttingDown)
			Common.exit("EXIT_CODE_UNHANDLED_PROMISE", CONF_EXIT_CODES.EXIT_CODE_UNHANDLED_PROMISE);
		(console as Console).error({ reason }); // This prints error with stack included (as for normal errors)
		theLogger.error("unhandledRejection", "unknown", exitHandler, undefined, reason);

		try {
			await performExit(true, undefined, reason);
			Common.exit("EXIT_CODE_UNHANDLED_EXCEPTION", CONF_EXIT_CODES.EXIT_CODE_UNHANDLED_EXCEPTION);
		} catch (error) {
			theLogger.error("Unhandled rejection cleanup failed", "unhandledRejection", exitHandler, undefined, error);
			Common.exit("EXIT_CODE_UNHANDLED_PROMISE", CONF_EXIT_CODES.EXIT_CODE_UNHANDLED_PROMISE);
		}
	});

	/**
	 * The 'uncaughtException' event is emitted when an uncaught JavaScript exception bubbles all the way back
	 * to the event loop. * By default, Node.js handles such exceptions by printing the stack trace to stderr
	 * and exiting with code 1, overriding any previously set Common.exitCode. Adding a handler for the
	 * 'uncaughtException' event overrides this default behavior. Alternatively, change the Common.exitCode
	 * in the 'uncaughtException' handler which will result in the process exiting with the provided exit code.
	 * Otherwise, in the presence of such handler the process will exit with 0.
	 *
	 * once to prefent error loops
	 *
	 * Details: https://nodejs.org/docs/latest-v12.x/api/process.html#process_event_uncaughtexception
	 */
	process.on("uncaughtException", async (error: Error): Promise<void> => {
		debugger;
		if (shuttingDown)
			Common.exit("EXIT_CODE_UNHANDLED_EXCEPTION", CONF_EXIT_CODES.EXIT_CODE_UNHANDLED_EXCEPTION);

		(console as Console).error({ error }); // This prints error with stack included (as for normal errors)
		theLogger.error("uncaughtException", "unknown", exitHandler, undefined, error);
		// As process exit wont trigger beforeExit
		try {
			await performExit(true, error);
			Common.exit("EXIT_CODE_UNHANDLED_EXCEPTION", CONF_EXIT_CODES.EXIT_CODE_UNHANDLED_EXCEPTION);
		} catch (error) {
			theLogger.error("Unhandled rejection cleanup failed", "unhandledRejection", exitHandler, undefined, error);
			Common.exit("EXIT_CODE_UNHANDLED_EXCEPTION", CONF_EXIT_CODES.EXIT_CODE_UNHANDLED_EXCEPTION);
		}
	});

	process.on("beforeExit", async (code: number): Promise<void> => {
		if (shuttingDown)
			return;
		theLogger.error(`Process beforeExit`, "beforeExit", exitHandler, { code });
		await performExit();
		Common.exit("EXIT_CODE_UNHANDLED_EXCEPTION", CONF_EXIT_CODES.EXIT_CODE_UNHANDLED_EXCEPTION);
	});

	process.on("exit", async (code: number): Promise<void> => {
		// Only synchronous calls
		theLogger.error(`Process exit`, "exit", exitHandler, { code });
		// TODO: ado clarify if worth (delay of restart)
		await performExit();
	});

	/**
	 * SIGTERM is normally sent by a process monitor to tell Node.js to expect a successful termination.
	 * If you're running systemd or upstart to manage your Node application, and you stop the service,
	 * it sends a SIGTERM event so that you can handle the process shutdown.
	 */
	process.on("SIGTERM", async (signal: NodeJS.Signals): Promise<void> => {
		theLogger.error(`ECS exit`, "SIGTERM", exitHandler, { signal: "SIGTERM" });
		await performExit();
		Common.exit("SIGTERM", 15);
	});

	/**
	 *
	 */
	process.on("SIGHUP", async (signal: NodeJS.Signals): Promise<void> => {
		theLogger.error(`ECS exit`, "SIGHUP", exitHandler, { signal: "SIGHUP" });
		await performExit();
		Common.exit("SIGHUP", 1);
	});

	/**
	 * SIGINT is emitted when a Node.js process is interrupted, usually as the result of a
	 * control-C (^-C) keyboard event. You can also capture that event and do some work around it.
	 */
	process.on("SIGINT", async (signal: NodeJS.Signals): Promise<void> => {
		theLogger.error(`ECS exit`, "SIGINT", exitHandler, { signal: "SIGINT" });
		await performExit();
		Common.exit("SIGINT", 0);
	});
};

export {
	initExitHandler,
	CONF_EXIT_CODES
};
