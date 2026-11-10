// Sample logger. pino writes the lines. The method names match the ROSE stub and the decorators.
import fs from "node:fs";
import path from "node:path";
import pino from "pino";

/**
 * Class name and optional fields attached to one log line.
 * getLogData returns this. The decorator passes either this object or a callback that produces it.
 */
export interface ILogData {
	className: string;
	classProps?: { [propName: string]: unknown };
}

/**
 * Object that can supply ILogData. Class instances pass themselves as the third logger argument.
 */
export interface ILogCallback {
	getLogData(): ILogData;
}

/**
 * One line after the optional storage callback has added request context.
 * LogLocalStorage.callback reads and returns this.
 */
export interface IFinalLogData {
	className?: string;
	method?: string;
	classProps?: { [propName: string]: unknown };
	message?: string;
	meta?: unknown;
	exception?: unknown;
}

/**
 * Settings app.ts passes to init. Console and file can both be on.
 */
export interface IELoggerSettings {
	fileLog?: { logFilename: string; logDirectory: string };
	consoleLog?: { logConsole: boolean };
	infrastructure?: { environment: string; servername: string; role: string; role_instance: number };
	logLevel: "error" | "warn" | "info" | "debug";
}

/**
 * pino-backed logger for the node-server sample.
 * Call init before the first log. setCallback runs on every line so request storage can add its fields.
 * exit flushes the file stream during process shutdown.
 */
export class ELogger {
	private logger?: pino.Logger;
	private destinations: Array<{ flushSync?: () => void }> = [];
	private enrich?: (logData: IFinalLogData) => IFinalLogData;

	/**
	 * Opens the console and file streams.
	 * Called once from app.ts after the config singleton exists.
	 */
	public init(settings: IELoggerSettings): void {
		const streams: pino.StreamEntry[] = [];
		this.destinations = [];
		if (settings.consoleLog?.logConsole !== false)
			streams.push({ stream: process.stdout });
		if (settings.fileLog?.logDirectory && settings.fileLog.logFilename) {
			fs.mkdirSync(settings.fileLog.logDirectory, { recursive: true });
			const dest = pino.destination({
				dest: path.join(settings.fileLog.logDirectory, settings.fileLog.logFilename),
				mkdir: true,
				sync: false,
			});
			this.destinations.push(dest);
			streams.push({ stream: dest });
		}
		const options: pino.LoggerOptions = {
			level: settings.logLevel,
			base: settings.infrastructure ?? null,
		};
		this.logger = streams.length > 0 ? pino(options, pino.multistream(streams)) : pino(options);
	}

	/**
	 * Installs the hook that adds async-local request fields.
	 * LogLocalStorage.init calls this. The hook runs on the logging thread before pino writes.
	 */
	public setCallback(enrich: (logData: IFinalLogData) => IFinalLogData): void {
		this.enrich = enrich;
	}

	/**
	 * Flushes file destinations.
	 * The exit handler awaits this before the process ends so the last lines reach disk.
	 */
	public async exit(): Promise<void> {
		for (const dest of this.destinations) {
			try {
				dest.flushSync?.();
			} catch {
				// The stream is already closed.
			}
		}
	}

	/** Error line. Same argument order as the ROSE logger and the decorators. */
	public error(msg: string, calling_method: string, logDataOrCallback?: ILogData | ILogCallback, meta?: unknown, exception?: unknown): void {
		this.write("error", msg, calling_method, logDataOrCallback, meta, exception);
	}

	/** Warning line. Same argument order as the ROSE logger and the decorators. */
	public warn(msg: string, calling_method: string, logDataOrCallback?: ILogData | ILogCallback, meta?: unknown, exception?: unknown): void {
		this.write("warn", msg, calling_method, logDataOrCallback, meta, exception);
	}

	/** Info line. Same argument order as the ROSE logger and the decorators. */
	public info(msg: string, calling_method: string, logDataOrCallback?: ILogData | ILogCallback, meta?: unknown, exception?: unknown): void {
		this.write("info", msg, calling_method, logDataOrCallback, meta, exception);
	}

	/** Debug line. Same argument order as the ROSE logger and the decorators. */
	public debug(msg: string, calling_method: string, logDataOrCallback?: ILogData | ILogCallback, meta?: unknown, exception?: unknown): void {
		this.write("debug", msg, calling_method, logDataOrCallback, meta, exception);
	}

	/**
	 * Builds one record and writes it at the given pino level.
	 * A callback third argument is resolved through getLogData. The enrich hook then adds request context.
	 */
	private write(
		level: "error" | "warn" | "info" | "debug",
		msg: string,
		calling_method: string,
		logDataOrCallback?: ILogData | ILogCallback,
		meta?: unknown,
		exception?: unknown,
	): void {
		if (!this.logger)
			return;
		const data = this.resolveData(logDataOrCallback);
		let record: IFinalLogData = {
			className: data?.className,
			method: calling_method,
			classProps: data?.classProps,
			message: msg,
			meta,
			exception: exception instanceof Error ? { name: exception.name, message: exception.message, stack: exception.stack } : exception,
		};
		if (this.enrich)
			record = this.enrich(record);
		this.logger[level](record, msg);
	}

	/**
	 * Reads ILogData from a plain object or from getLogData.
	 * Decorators pass the class instance. Direct calls pass { className }.
	 */
	private resolveData(logDataOrCallback?: ILogData | ILogCallback): ILogData | undefined {
		if (!logDataOrCallback)
			return undefined;
		if (typeof (logDataOrCallback as ILogCallback).getLogData === "function")
			return (logDataOrCallback as ILogCallback).getLogData();
		return logDataOrCallback as ILogData;
	}
}
